/*
 * Academic License - for use in teaching, academic research, and meeting
 * course requirements at degree granting institutions only.  Not for
 * government, commercial, or other organizational use.
 *
 * File: state_machine_types.h
 *
 * Code generated for Simulink model 'state_machine'.
 *
 * Model version                  : 1.9
 * Simulink Coder version         : 26.1 (R2026a) 20-Nov-2025
 * C/C++ source code generated on : Fri Jul  3 12:46:08 2026
 *
 * Target selection: ert.tlc
 * Embedded hardware selection: Intel->x86-64 (Windows64)
 * Code generation objectives: Unspecified
 * Validation result: Not run
 */

#ifndef state_machine_types_h_
#define state_machine_types_h_
#include "rtwtypes.h"
#ifndef DEFINED_TYPEDEF_FOR_Mode_
#define DEFINED_TYPEDEF_FOR_Mode_

typedef enum {
  UNPRESSED = 0,                       /* Default value */
  PRESSED,
  HOLD
} Mode;

#endif

/* Forward declaration for rtModel */
typedef struct tag_RTM_state_machine_T RT_MODEL_state_machine_T;

#endif                                 /* state_machine_types_h_ */

/*
 * File trailer for generated code.
 *
 * [EOF]
 */
