/* Harness interface of the cost-bucket prover.
 *
 * A harness is a C file with a `main` that:
 *   1. assigns every input of the top function with HUX_INPUT(type, name, lvalue), optionally
 *      constrained with __VERIFIER_assume,
 *   2. calls HUX_START() right before the top function, then calls it,
 *   3. ends with HUX_CHECK_BUCKET().
 *
 * In HUX_INPUT, `type` is the suffix of a __VERIFIER_nondet_<type> function (bool, char, uchar,
 * short, ushort, int, uint, long, ulong, longlong, ulonglong, float, double), `name` identifies the
 * input (the C expression of its trace column) and `lvalue` is what the input is assigned to.
 *
 * Every function defined in the sources is instrumented, so the harness may call other functions
 * before the top one (e.g. a model's initialize function): HUX_START() resets the counter, so that
 * only the top function's call is measured.
 *
 * The same file is compiled in three modes:
 *   - prove  (default): HUX_INPUT assigns a nondet value of the input's type (see hux_nondet_*
 *     below), and HUX_CHECK_BUCKET() is reachable-error iff the cost is in [HUX_LO, HUX_HI). HUX_HI
 *     undefined means the bucket is unbounded above.
 *   - replay (-DHUX_REPLAY): the same nondet calls return the counterexample's values, HUX_INPUT
 *     prints each input as executed, and HUX_CHECK_BUCKET() prints the measured cost.
 *   - sample (-DHUX_SAMPLE): runtime/sample_rt.c runs the harness once per row of a rows file.
 *     HUX_INPUT reads the input from the row by its name, a false __VERIFIER_assume rejects the row,
 *     and HUX_CHECK_BUCKET() reports the measured cost.
 */
#ifndef HUX_H
#define HUX_H

#include <stdbool.h>
#include <stdint.h>

extern int64_t __hux_cost; /* defined by the cost-counter pass */

/* Resets the cost counter: call it right before the top function. */
#define HUX_START() (__hux_cost = 0)

extern void __VERIFIER_assume(int);

extern bool __VERIFIER_nondet_bool(void);
extern char __VERIFIER_nondet_char(void);
extern unsigned char __VERIFIER_nondet_uchar(void);
extern short __VERIFIER_nondet_short(void);
extern unsigned short __VERIFIER_nondet_ushort(void);
extern int __VERIFIER_nondet_int(void);
extern unsigned int __VERIFIER_nondet_uint(void);
extern long __VERIFIER_nondet_long(void);
extern unsigned long __VERIFIER_nondet_ulong(void);
extern long long __VERIFIER_nondet_longlong(void);
extern unsigned long long __VERIFIER_nondet_ulonglong(void);
extern float __VERIFIER_nondet_float(void);
extern double __VERIFIER_nondet_double(void);

#if !defined(HUX_SAMPLE)

#include <limits.h>

/* A nondet value of each input type, for the prove and replay modes (replay makes the same calls,
 * so that the counterexample's values reach the same inputs).
 *
 * SeaHorn reads integers as mathematical integers, without their type's range: a uint8_t input
 * from __VERIFIER_nondet_uchar may be 1000 for the prover, which then cannot prove a bound that
 * holds because of the type. So an integer type narrower than 64 bits is drawn from a wider one
 * and assumed in its range. 64-bit integers, bools and floating-point values are drawn as they
 * are. */
static inline char hux_nondet_char(void) {
  int v = __VERIFIER_nondet_int();
  __VERIFIER_assume(v >= CHAR_MIN && v <= CHAR_MAX);
  return (char)v;
}
static inline unsigned char hux_nondet_uchar(void) {
  int v = __VERIFIER_nondet_int();
  __VERIFIER_assume(v >= 0 && v <= UCHAR_MAX);
  return (unsigned char)v;
}
static inline short hux_nondet_short(void) {
  int v = __VERIFIER_nondet_int();
  __VERIFIER_assume(v >= SHRT_MIN && v <= SHRT_MAX);
  return (short)v;
}
static inline unsigned short hux_nondet_ushort(void) {
  int v = __VERIFIER_nondet_int();
  __VERIFIER_assume(v >= 0 && v <= USHRT_MAX);
  return (unsigned short)v;
}
static inline int hux_nondet_int(void) {
  long long v = __VERIFIER_nondet_longlong();
  __VERIFIER_assume(v >= INT_MIN && v <= INT_MAX);
  return (int)v;
}
static inline unsigned int hux_nondet_uint(void) {
  long long v = __VERIFIER_nondet_longlong();
  __VERIFIER_assume(v >= 0 && v <= UINT_MAX);
  return (unsigned int)v;
}
#define hux_nondet_bool __VERIFIER_nondet_bool
#define hux_nondet_long __VERIFIER_nondet_long
#define hux_nondet_ulong __VERIFIER_nondet_ulong
#define hux_nondet_longlong __VERIFIER_nondet_longlong
#define hux_nondet_ulonglong __VERIFIER_nondet_ulonglong
#define hux_nondet_float __VERIFIER_nondet_float
#define hux_nondet_double __VERIFIER_nondet_double

#endif

#if defined(HUX_REPLAY)

#include <stdio.h>

/* Prints an input as executed, with its kind: int, uint or float. */
static inline void hux_record_int_(const char *name, long long v) {
  printf("input %s int %lld\n", name, v);
}
static inline void hux_record_uint_(const char *name, unsigned long long v) {
  printf("input %s uint %llu\n", name, v);
}
static inline void hux_record_float_(const char *name, double v) {
  printf("input %s float %.17g\n", name, v);
}
#define hux_record_bool(name, v) hux_record_uint_(name, (unsigned long long)(v))
#define hux_record_char(name, v) hux_record_int_(name, (long long)(v))
#define hux_record_uchar(name, v) hux_record_uint_(name, (unsigned long long)(v))
#define hux_record_short(name, v) hux_record_int_(name, (long long)(v))
#define hux_record_ushort(name, v) hux_record_uint_(name, (unsigned long long)(v))
#define hux_record_int(name, v) hux_record_int_(name, (long long)(v))
#define hux_record_uint(name, v) hux_record_uint_(name, (unsigned long long)(v))
#define hux_record_long(name, v) hux_record_int_(name, (long long)(v))
#define hux_record_ulong(name, v) hux_record_uint_(name, (unsigned long long)(v))
#define hux_record_longlong(name, v) hux_record_int_(name, (long long)(v))
#define hux_record_ulonglong(name, v) hux_record_uint_(name, (unsigned long long)(v))
#define hux_record_float(name, v) hux_record_float_(name, (double)(v))
#define hux_record_double(name, v) hux_record_float_(name, (double)(v))

#define HUX_INPUT(type, name, lvalue)                                                              \
  do {                                                                                             \
    (lvalue) = hux_nondet_##type();                                                                \
    hux_record_##type((name), (lvalue));                                                           \
  } while (0)
#define HUX_CHECK_BUCKET() printf("cost %lld\n", (long long)__hux_cost)

#elif defined(HUX_SAMPLE)

/* The input `name` of the current row (see runtime/sample_rt.c). */
extern bool hux_sample_bool(const char *name);
extern char hux_sample_char(const char *name);
extern unsigned char hux_sample_uchar(const char *name);
extern short hux_sample_short(const char *name);
extern unsigned short hux_sample_ushort(const char *name);
extern int hux_sample_int(const char *name);
extern unsigned int hux_sample_uint(const char *name);
extern long hux_sample_long(const char *name);
extern unsigned long hux_sample_ulong(const char *name);
extern long long hux_sample_longlong(const char *name);
extern unsigned long long hux_sample_ulonglong(const char *name);
extern float hux_sample_float(const char *name);
extern double hux_sample_double(const char *name);
/* Reports the measured cost of the current row. */
extern void hux_sample_cost(long long cost);

#define HUX_INPUT(type, name, lvalue) ((lvalue) = hux_sample_##type(name))
#define HUX_CHECK_BUCKET() hux_sample_cost((long long)__hux_cost)

/* The runtime provides `main` and runs the harness's, renamed, once per row. */
#define main hux_harness_main

#else

extern void __VERIFIER_error(void);

#ifndef HUX_LO
#error "HUX_LO must be defined in prove mode"
#endif

#define HUX_INPUT(type, name, lvalue) ((lvalue) = hux_nondet_##type())

#ifdef HUX_HI
#define HUX_CHECK_BUCKET()                                                                         \
  do {                                                                                             \
    if (__hux_cost >= (int64_t)(HUX_LO) && __hux_cost < (int64_t)(HUX_HI))                         \
      __VERIFIER_error();                                                                          \
  } while (0)
#else
#define HUX_CHECK_BUCKET()                                                                         \
  do {                                                                                             \
    if (__hux_cost >= (int64_t)(HUX_LO))                                                           \
      __VERIFIER_error();                                                                          \
  } while (0)
#endif

#endif

#endif /* HUX_H */
