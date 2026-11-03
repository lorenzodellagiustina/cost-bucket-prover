/* Replay fallbacks for nondet calls that the SeaHorn counterexample does not define.
 *
 * SeaHorn's counterexample harness defines __VERIFIER_nondet_<type> only for the integer and
 * pointer types it recorded; floating-point inputs are never recorded. These weak definitions are
 * overridden by the counterexample's strong ones. When one of them runs, the input was not
 * determined by the counterexample: it prints "unrecorded <type>" and returns 0, so the caller of
 * the replay can tell a witness that depends on such a default.
 */
#include <stdbool.h>
#include <stdio.h>

#define HUX_FALLBACK(type, name)                            \
  __attribute__((weak)) type __VERIFIER_nondet_##name(void) \
  {                                                         \
    printf("unrecorded " #name "\n");                       \
    return (type)0;                                         \
  }

HUX_FALLBACK(bool, bool)
HUX_FALLBACK(char, char)
HUX_FALLBACK(unsigned char, uchar)
HUX_FALLBACK(short, short)
HUX_FALLBACK(unsigned short, ushort)
HUX_FALLBACK(int, int)
HUX_FALLBACK(unsigned int, uint)
HUX_FALLBACK(long, long)
HUX_FALLBACK(unsigned long, ulong)
HUX_FALLBACK(long long, longlong)
HUX_FALLBACK(unsigned long long, ulonglong)
HUX_FALLBACK(float, float)
HUX_FALLBACK(double, double)
