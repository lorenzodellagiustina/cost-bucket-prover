/*
 * Academic License - for use in teaching, academic research, and meeting
 * course requirements at degree granting institutions only.  Not for
 * government, commercial, or other organizational use.
 *
 * File: state_machine.c
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
#include "rtwtypes.h"
#include "state_machine_private.h"
#include "rt_nonfinite.h"
#include "state_machine_types.h"

/* Named constants for Chart: '<Root>/Chart' */
#define state_machine_IN_calibration   ((uint8_T)1U)
#define state_machine_IN_lv            ((uint8_T)2U)
#define state_machine_IN_r2d           ((uint8_T)3U)
#define state_machine_IN_ts            ((uint8_T)4U)

/* Block signals (default storage) */
B_state_machine_T state_machine_B;

/* Block states (default storage) */
DW_state_machine_T state_machine_DW;

/* External inputs (root inport signals with default storage) */
ExtU_state_machine_T state_machine_U;

/* External outputs (root outports fed by signals with default storage) */
ExtY_state_machine_T state_machine_Y;

/* Real-time model */
static RT_MODEL_state_machine_T state_machine_M_;
RT_MODEL_state_machine_T *const state_machine_M = &state_machine_M_;

/* Output and update for function-call system: '<S2>/simfcn' */
boolean_T state_machine_simfcn(const real_T rtu_battery_voltage1[3])
{
  boolean_T rty_soc_0;
  int32_T i;

  /* Outputs for Atomic SubSystem: '<S3>/RegressionNeuralNetwork Predict' */
  /* Switch: '<S15>/Switch' incorporates:
   *  Constant: '<S15>/maxPriorClassInd'
   *  Constant: '<S9>/bias'
   *  DataTypeConversion: '<S9>/Data Type Conversion'
   *  Logic: '<S15>/Logical Operator'
   *  RelationalOperator: '<S15>/IsNaN'
   *  Sum: '<S9>/Add'
   */
  if (rtIsNaN(rtu_battery_voltage1[0]) || rtIsNaN(rtu_battery_voltage1[1]) ||
      rtIsNaN(rtu_battery_voltage1[2])) {
    rty_soc_0 = true;
  } else {
    real_T rtu_battery_voltage1_0;
    real_T rtu_battery_voltage1_1;
    real_T rtu_battery_voltage1_2;
    real_T tmp;

    /* Product: '<S9>/Product' */
    tmp = 0.0;

    /* Product: '<S10>/Product' */
    rtu_battery_voltage1_0 = rtu_battery_voltage1[1];
    rtu_battery_voltage1_1 = rtu_battery_voltage1[0];
    rtu_battery_voltage1_2 = rtu_battery_voltage1[2];
    for (i = 0; i < 5; i++) {
      real_T rtb_Switch_p_0;

      /* Sum: '<S10>/Add' incorporates:
       *  Constant: '<S10>/bias'
       *  Constant: '<S10>/weights'
       *  Product: '<S10>/Product'
       *  Switch: '<S14>/Switch'
       */
      rtb_Switch_p_0 = ((state_machine_ConstP.weights_Value[3 * i + 1] *
                         rtu_battery_voltage1_0 +
                         state_machine_ConstP.weights_Value[3 * i] *
                         rtu_battery_voltage1_1) +
                        state_machine_ConstP.weights_Value[3 * i + 2] *
                        rtu_battery_voltage1_2) +
        state_machine_ConstP.bias_Value[i];

      /* Product: '<S9>/Product' incorporates:
       *  Constant: '<S12>/Constant'
       *  Constant: '<S9>/weights'
       *  RelationalOperator: '<S12>/Relational Operator'
       *  RelationalOperator: '<S14>/IsNaN'
       *  Switch: '<S12>/Switch'
       *  Switch: '<S14>/Switch'
       */
      if (rtIsNaN(rtb_Switch_p_0)) {
        rtb_Switch_p_0 = (rtNaN);
      } else if (!(rtb_Switch_p_0 > 0.0)) {
        rtb_Switch_p_0 = 0.0;
      }

      tmp += rtb_Switch_p_0 * state_machine_ConstP.weights_Value_k[i];
    }

    rty_soc_0 = (tmp + 0.5037891136626571 != 0.0);
  }

  /* End of Switch: '<S15>/Switch' */
  /* End of Outputs for SubSystem: '<S3>/RegressionNeuralNetwork Predict' */
  return rty_soc_0;
}

/* Model step function */
void state_machine_step(void)
{
  /* Chart: '<Root>/Chart' incorporates:
   *  Inport: '<Root>/accellerator'
   *  Inport: '<Root>/battery_voltage'
   *  Inport: '<Root>/brake'
   *  Inport: '<Root>/button'
   *  Inport: '<Root>/shutdown_closed'
   */
  if (state_machine_DW.is_active_c3_state_machine == 0) {
    state_machine_DW.is_active_c3_state_machine = 1U;
    state_machine_DW.is_c3_state_machine = state_machine_IN_lv;
  } else {
    switch (state_machine_DW.is_c3_state_machine) {
     case state_machine_IN_calibration:
      state_machine_DW.is_c3_state_machine = state_machine_IN_lv;
      break;

     case state_machine_IN_lv:
      if (state_machine_U.button == (real_T)HOLD) {
        state_machine_DW.is_c3_state_machine = state_machine_IN_calibration;
      } else if ((state_machine_U.button == (real_T)PRESSED) &&
                 (state_machine_U.shutdown_closed != 0.0)) {
        state_machine_DW.is_c3_state_machine = state_machine_IN_ts;
      }
      break;

     case state_machine_IN_r2d:
      state_machine_DW.is_c3_state_machine = state_machine_IN_lv;
      break;

     default:
      /* case IN_ts: */
      if (!(state_machine_U.shutdown_closed != 0.0)) {
        state_machine_DW.is_c3_state_machine = state_machine_IN_lv;
      } else if ((state_machine_U.accellerator == 0.0) && (state_machine_U.brake
                  == 100.0)) {
        state_machine_DW.is_c3_state_machine = state_machine_IN_r2d;

        /* Outputs for Function Call SubSystem: '<S2>/simfcn' */
        state_machine_simfcn(state_machine_U.battery_voltage);

        /* End of Outputs for SubSystem: '<S2>/simfcn' */
      }
      break;
    }
  }

  /* End of Chart: '<Root>/Chart' */
}

/* Model initialize function */
void state_machine_initialize(void)
{
  /* (no initialization code required) */
}

/* Model terminate function */
void state_machine_terminate(void)
{
  /* (no terminate code required) */
}

/*
 * File trailer for generated code.
 *
 * [EOF]
 */
