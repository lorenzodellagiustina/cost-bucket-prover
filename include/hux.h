/* Harness interface of the cost-bucket prover.
 *
 * A harness is a C file with a `main` that:
 *   1. assigns every input of the top function from a __VERIFIER_nondet_<type>() call,
 *   2. records each input with HUX_RECORD_<kind>(name, value),
 *   3. calls the top function,
 *   4. ends with HUX_CHECK_BUCKET().
 *
 * The same file is compiled in two modes:
 *   - prove  (default): HUX_CHECK_BUCKET() is reachable-error iff the cost is in [HUX_LO, HUX_HI);
 *     HUX_RECORD_* do nothing. HUX_HI undefined means the bucket is unbounded above.
 *   - replay (-DHUX_REPLAY): the nondet calls return the counterexample's values, HUX_RECORD_*
 *     print each input as executed, and HUX_CHECK_BUCKET() prints the measured cost.
 */
#ifndef HUX_H
#define HUX_H

#include <stdbool.h>
#include <stdint.h>

extern int64_t __hux_cost; /* defined by the cost-counter pass */

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

#ifdef HUX_REPLAY

#include <stdio.h>

#define HUX_RECORD_INT(name, v) printf("input %s int %lld\n", (name), (long long)(v))
#define HUX_RECORD_UINT(name, v) printf("input %s uint %llu\n", (name), (unsigned long long)(v))
#define HUX_RECORD_FLOAT(name, v) printf("input %s float %.17g\n", (name), (double)(v))
#define HUX_CHECK_BUCKET() printf("cost %lld\n", (long long)__hux_cost)

#else

extern void __VERIFIER_error(void);

#ifndef HUX_LO
#error "HUX_LO must be defined in prove mode"
#endif

#define HUX_RECORD_INT(name, v) ((void)0)
#define HUX_RECORD_UINT(name, v) ((void)0)
#define HUX_RECORD_FLOAT(name, v) ((void)0)

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

#endif /* HUX_REPLAY */

#endif /* HUX_H */
