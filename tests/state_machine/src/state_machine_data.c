/*
 * Academic License - for use in teaching, academic research, and meeting
 * course requirements at degree granting institutions only.  Not for
 * government, commercial, or other organizational use.
 *
 * File: state_machine_data.c
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

#include "state_machine.h"

/* Constant parameters (default storage) */
const ConstP_state_machine_T state_machine_ConstP = {
  /* Expression: LayerBias
   * Referenced by: '<S10>/bias'
   */
  { -0.35337958975559042, 0.0, -0.35404676355936315, 0.0, -0.49231735917629815 },

  /* Expression: LayerWeights
   * Referenced by: '<S10>/weights'
   */
  { 0.99073249044524869, -0.17424321120326436, -0.34422142733948841,
    -0.22923186949905927, -0.1935192116720085, -0.60302123938108887,
    -0.65401114770130531, -0.10998479620451178, 0.2507721987617248,
    -0.71591454037880053, -0.46946673872230754, -0.69177014189007169,
    0.2049739514879389, -0.038716200058044152, -0.58437631861330563 },

  /* Expression: learnerParams.PredictWeightsOut
   * Referenced by: '<S9>/weights'
   */
  { -0.82789527943863894, 0.061744514055856785, -0.74336817814629919,
    -0.18936916023881742, -0.64600072319525748 }
};

/*
 * File trailer for generated code.
 *
 * [EOF]
 */
