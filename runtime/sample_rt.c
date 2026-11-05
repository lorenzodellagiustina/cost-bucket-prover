/* Sample runtime: runs the harness (compiled with -DHUX_SAMPLE) once per row of a rows file, each
 * row in a fresh process, so that every row starts from the same initial global state.
 *
 * Usage: sampler ROWS RESULTS TIMEOUT PRINT_INPUTS
 *
 * ROWS is tab-separated: the first line holds the input names (the names of the harness's
 * HUX_INPUT), each following line one row of values. Integers are decimal, floats anything strtod
 * accepts (including inf and nan), bools 0, 1, true or false.
 *
 * RESULTS is YAML, one entry per row run, in order:
 *   status: ok | input_error
 *   message: null, or what is wrong with the rows file (the run stops at the first such row)
 *   rows:
 *     - {status: ok, cost: 15}       ok | rejected | crash | timeout
 *   A row is `rejected` when a __VERIFIER_assume is false, `crash` when the harness exits abnormally
 *   or without HUX_CHECK_BUCKET(), `timeout` after TIMEOUT seconds. With PRINT_INPUTS=1, each ok row
 *   also lists its inputs as executed, in the format of the replay.
 *
 * The harness's own output goes to stderr.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int hux_harness_main(void);

/* Exit codes of a row's process, besides 0. */
enum { EXIT_REJECTED = 97, EXIT_INPUT_ERROR = 98 };

static char **names;  /* the header */
static size_t width;  /* number of columns */
static char **cells;  /* the current row */
static FILE *report;  /* in a row's process: the pipe to the parent */
static bool print_inputs;
static bool reported_cost;

/* Splits `line` (without its newline) at tabs, in place. */
static char **split(char *line, size_t *n) {
  size_t cap = 8;
  char **out = malloc(cap * sizeof *out);
  *n = 0;
  for (char *p = line;;) {
    if (*n == cap)
      out = realloc(out, (cap *= 2) * sizeof *out);
    out[(*n)++] = p;
    char *tab = strchr(p, '\t');
    if (!tab)
      break;
    *tab = '\0';
    p = tab + 1;
  }
  return out;
}

/* In a row's process: reports that the rows file is wrong and stops. */
static void input_error(const char *format, ...) {
  va_list args;
  va_start(args, format);
  fprintf(report, "error ");
  vfprintf(report, format, args);
  fprintf(report, "\n");
  va_end(args);
  fflush(report);
  _exit(EXIT_INPUT_ERROR);
}

static const char *cell(const char *name) {
  for (size_t i = 0; i < width; ++i)
    if (strcmp(names[i], name) == 0)
      return cells[i];
  input_error("no column '%s'", name);
  return NULL;
}

static long long signed_value(const char *name, long long min, long long max) {
  const char *text = cell(name);
  char *end;
  errno = 0;
  long long v = strtoll(text, &end, 10);
  if (errno || end == text || *end || v < min || v > max)
    input_error("'%s' = '%s' is not a value of its type", name, text);
  if (print_inputs)
    fprintf(report, "input %s int %lld\n", name, v);
  return v;
}

static unsigned long long unsigned_value(const char *name, unsigned long long max) {
  const char *text = cell(name);
  char *end;
  errno = 0;
  unsigned long long v = strtoull(text, &end, 10);
  if (errno || end == text || *end || strchr(text, '-') || v > max)
    input_error("'%s' = '%s' is not a value of its type", name, text);
  if (print_inputs)
    fprintf(report, "input %s uint %llu\n", name, v);
  return v;
}

static double float_value(const char *name, bool single) {
  const char *text = cell(name);
  char *end;
  double v = single ? (double)strtof(text, &end) : strtod(text, &end);
  if (end == text || *end)
    input_error("'%s' = '%s' is not a value of its type", name, text);
  if (print_inputs)
    fprintf(report, "input %s float %.17g\n", name, v);
  return v;
}

bool hux_sample_bool(const char *name) {
  const char *text = cell(name);
  bool v;
  if (strcmp(text, "0") == 0 || strcmp(text, "false") == 0)
    v = false;
  else if (strcmp(text, "1") == 0 || strcmp(text, "true") == 0)
    v = true;
  else
    input_error("'%s' = '%s' is not a bool", name, text);
  if (print_inputs)
    fprintf(report, "input %s uint %d\n", name, v);
  return v;
}
char hux_sample_char(const char *name) { return (char)signed_value(name, CHAR_MIN, CHAR_MAX); }
unsigned char hux_sample_uchar(const char *name) {
  return (unsigned char)unsigned_value(name, UCHAR_MAX);
}
short hux_sample_short(const char *name) { return (short)signed_value(name, SHRT_MIN, SHRT_MAX); }
unsigned short hux_sample_ushort(const char *name) {
  return (unsigned short)unsigned_value(name, USHRT_MAX);
}
int hux_sample_int(const char *name) { return (int)signed_value(name, INT_MIN, INT_MAX); }
unsigned int hux_sample_uint(const char *name) {
  return (unsigned int)unsigned_value(name, UINT_MAX);
}
long hux_sample_long(const char *name) { return (long)signed_value(name, LONG_MIN, LONG_MAX); }
unsigned long hux_sample_ulong(const char *name) {
  return (unsigned long)unsigned_value(name, ULONG_MAX);
}
long long hux_sample_longlong(const char *name) {
  return signed_value(name, LLONG_MIN, LLONG_MAX);
}
unsigned long long hux_sample_ulonglong(const char *name) {
  return unsigned_value(name, ULLONG_MAX);
}
float hux_sample_float(const char *name) { return (float)float_value(name, true); }
double hux_sample_double(const char *name) { return float_value(name, false); }

void hux_sample_cost(long long cost) {
  fprintf(report, "cost %lld\n", cost);
  reported_cost = true;
}

void __VERIFIER_assume(int condition) {
  if (!condition)
    _exit(EXIT_REJECTED);
}

/* Writes `text` as a YAML double-quoted string. */
static void yaml_string(FILE *out, const char *text) {
  fputc('"', out);
  for (; *text; ++text) {
    if (*text == '"' || *text == '\\')
      fputc('\\', out);
    fputc(*text, out);
  }
  fputc('"', out);
}

int main(int argc, char **argv) {
  if (argc != 5) {
    fprintf(stderr, "usage: %s ROWS RESULTS TIMEOUT PRINT_INPUTS\n", argv[0]);
    return 2;
  }
  FILE *rows = fopen(argv[1], "r");
  FILE *out = fopen(argv[2], "w");
  if (!rows || !out) {
    perror("cannot open the rows or the results file");
    return 2;
  }
  unsigned timeout = (unsigned)atoi(argv[3]);
  print_inputs = strcmp(argv[4], "1") == 0;

  char *header = NULL;
  size_t size = 0;
  ssize_t len = getline(&header, &size, rows);
  if (len <= 0) {
    fprintf(out, "status: input_error\nmessage: \"the rows file has no header\"\nrows: []\n");
    return 0;
  }
  if (header[len - 1] == '\n')
    header[len - 1] = '\0';
  names = split(header, &width);

  fprintf(out, "rows:\n");
  bool any = false;
  char *message = NULL;
  char *line = NULL;
  size = 0;
  for (size_t row = 1; (len = getline(&line, &size, rows)) > 0; ++row) {
    if (line[len - 1] == '\n')
      line[--len] = '\0';
    size_t n;
    cells = split(line, &n);
    if (n != width) {
      asprintf(&message, "row %zu has %zu values for %zu columns", row, n, width);
      break;
    }

    int fds[2];
    if (pipe(fds) != 0) {
      perror("pipe");
      return 2;
    }
    fflush(NULL);
    pid_t pid = fork();
    if (pid < 0) {
      perror("fork");
      return 2;
    }
    if (pid == 0) {
      close(fds[0]);
      report = fdopen(fds[1], "w");
      dup2(STDERR_FILENO, STDOUT_FILENO); /* the harness's own output goes to the log */
      alarm(timeout);
      hux_harness_main();
      fflush(stdout);
      fflush(report);
      _exit(reported_cost ? 0 : 1);
    }
    close(fds[1]);

    /* The row's report: its inputs, then its cost, or an error. */
    FILE *in = fdopen(fds[0], "r");
    char *inputs = NULL;
    size_t inputs_size = 0;
    FILE *inputs_out = open_memstream(&inputs, &inputs_size);
    bool first_input = true;
    long long cost = 0;
    bool has_cost = false;
    char *report_line = NULL;
    size_t report_size = 0;
    ssize_t report_len;
    while ((report_len = getline(&report_line, &report_size, in)) > 0) {
      if (report_line[report_len - 1] == '\n')
        report_line[report_len - 1] = '\0';
      char name[4096], kind[16], value[256];
      if (sscanf(report_line, "cost %lld", &cost) == 1) {
        has_cost = true;
      } else if (strncmp(report_line, "error ", 6) == 0) {
        free(message);
        message = strdup(report_line + 6);
      } else if (sscanf(report_line, "input %4095s %15s %255s", name, kind, value) == 3) {
        fprintf(inputs_out, "%s{ name: ", first_input ? "" : ", ");
        first_input = false;
        yaml_string(inputs_out, name);
        fprintf(inputs_out, ", kind: %s, value: ", kind);
        yaml_string(inputs_out, value);
        fprintf(inputs_out, " }");
      }
    }
    fclose(in);
    fclose(inputs_out);
    int wstatus;
    waitpid(pid, &wstatus, 0);

    const char *status;
    if (WIFEXITED(wstatus) && WEXITSTATUS(wstatus) == EXIT_INPUT_ERROR) {
      free(inputs);
      break;
    } else if (WIFEXITED(wstatus) && WEXITSTATUS(wstatus) == EXIT_REJECTED)
      status = "rejected";
    else if (WIFSIGNALED(wstatus) && WTERMSIG(wstatus) == SIGALRM)
      status = "timeout";
    else if (WIFEXITED(wstatus) && WEXITSTATUS(wstatus) == 0 && has_cost)
      status = "ok";
    else
      status = "crash";

    if (strcmp(status, "ok") == 0) {
      fprintf(out, "  - { status: ok, cost: %lld", cost);
      if (print_inputs)
        fprintf(out, ", inputs: [%s]", inputs);
      fprintf(out, " }\n");
    } else {
      fprintf(out, "  - { status: %s }\n", status);
    }
    any = true;
    free(inputs);
    free(cells);
  }
  if (!any)
    fprintf(out, "  []\n");
  if (message) {
    fprintf(out, "status: input_error\nmessage: ");
    yaml_string(out, message);
    fprintf(out, "\n");
  } else {
    fprintf(out, "status: ok\nmessage: null\n");
  }
  return 0;
}
