/*
 * Academic License - for use in teaching, academic research, and meeting
 * course requirements at degree granting institutions only.  Not for
 * government, commercial, or other organizational use.
 *
 * File: state_machine.h
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

#ifndef state_machine_h_
#define state_machine_h_
#ifndef state_machine_COMMON_INCLUDES_
#define state_machine_COMMON_INCLUDES_
#include "rtwtypes.h"
#include "rt_nonfinite.h"
#include "math.h"
#endif                                 /* state_machine_COMMON_INCLUDES_ */

#include "state_machine_types.h"
#include "rtGetNaN.h"

/* Macros for accessing real-time model data structure */
#ifndef rtmGetErrorStatus
#define rtmGetErrorStatus(rtm)         ((rtm)->errorStatus)
#endif

#ifndef rtmSetErrorStatus
#define rtmSetErrorStatus(rtm, val)    ((rtm)->errorStatus = (val))
#endif

/* Block signals (default storage) */
typedef struct {
  real_T inverter_voltage_k;           /* '<Root>/Chart' */
} B_state_machine_T;

/* Block states (default storage) for system '<Root>' */
typedef struct {
  uint8_T is_active_c3_state_machine;  /* '<Root>/Chart' */
  uint8_T is_c3_state_machine;         /* '<Root>/Chart' */
} DW_state_machine_T;

/* Constant parameters (default storage) */
typedef struct {
  /* Expression: LayerBias
   * Referenced by: '<S10>/bias'
   */
  real_T bias_Value[5];

  /* Expression: LayerWeights
   * Referenced by: '<S10>/weights'
   */
  real_T weights_Value[15];

  /* Expression: learnerParams.PredictWeightsOut
   * Referenced by: '<S9>/weights'
   */
  real_T weights_Value_k[5];
} ConstP_state_machine_T;

/* External inputs (root inport signals with default storage) */
typedef struct {
  real_T brake;                        /* '<Root>/brake' */
  real_T accellerator;                 /* '<Root>/accellerator' */
  real_T button;                       /* '<Root>/button' */
  real_T shutdown_closed;              /* '<Root>/shutdown_closed' */
  real_T battery_voltage[3];           /* '<Root>/battery_voltage' */
} ExtU_state_machine_T;

/* External outputs (root outports fed by signals with default storage) */
typedef struct {
  real_T inverter_voltage;             /* '<Root>/inverter_voltage' */
} ExtY_state_machine_T;

/* Real-time Model Data Structure */
struct tag_RTM_state_machine_T {
  const char_T * volatile errorStatus;
};

/* Block signals (default storage) */
extern B_state_machine_T state_machine_B;

/* Block states (default storage) */
extern DW_state_machine_T state_machine_DW;

/* External inputs (root inport signals with default storage) */
extern ExtU_state_machine_T state_machine_U;

/* External outputs (root outports fed by signals with default storage) */
extern ExtY_state_machine_T state_machine_Y;

/* Constant parameters (default storage) */
extern const ConstP_state_machine_T state_machine_ConstP;

/* Model entry point functions */
extern void state_machine_initialize(void);
extern void state_machine_step(void);
extern void state_machine_terminate(void);

/* Real-time Model object */
extern RT_MODEL_state_machine_T *const state_machine_M;

/*-
 * These blocks were eliminated from the model due to optimizations:
 *
 * Block '<S4>/Reshape' : Reshape block reduction
 * Block '<S9>/Reshape' : Reshape block reduction
 */

/*-
 * The generated code includes comments that allow you to trace directly
 * back to the appropriate location in the model.  The basic format
 * is <system>/block_name, where system is the system number (uniquely
 * assigned by Simulink) and block_name is the name of the block.
 *
 * Use the MATLAB hilite_system command to trace the generated code back
 * to the model.  For example,
 *
 * hilite_system('<S3>')    - opens system 3
 * hilite_system('<S3>/Kp') - opens and selects block Kp which resides in S3
 *
 * Here is the system hierarchy for this model
 *
 * '<Root>' : 'state_machine'
 * '<S1>'   : 'state_machine/Chart'
 * '<S2>'   : 'state_machine/Chart/r2d'
 * '<S3>'   : 'state_machine/Chart/r2d/simfcn'
 * '<S4>'   : 'state_machine/Chart/r2d/simfcn/RegressionNeuralNetwork Predict'
 * '<S5>'   : 'state_machine/Chart/r2d/simfcn/RegressionNeuralNetwork Predict/getScore'
 * '<S6>'   : 'state_machine/Chart/r2d/simfcn/RegressionNeuralNetwork Predict/processNanResponse'
 * '<S7>'   : 'state_machine/Chart/r2d/simfcn/RegressionNeuralNetwork Predict/standardize'
 * '<S8>'   : 'state_machine/Chart/r2d/simfcn/RegressionNeuralNetwork Predict/getScore/hiddenLayers'
 * '<S9>'   : 'state_machine/Chart/r2d/simfcn/RegressionNeuralNetwork Predict/getScore/outputLayer'
 * '<S10>'  : 'state_machine/Chart/r2d/simfcn/RegressionNeuralNetwork Predict/getScore/hiddenLayers/hiddenLayer1'
 * '<S11>'  : 'state_machine/Chart/r2d/simfcn/RegressionNeuralNetwork Predict/getScore/hiddenLayers/hiddenLayer1/Activation'
 * '<S12>'  : 'state_machine/Chart/r2d/simfcn/RegressionNeuralNetwork Predict/getScore/hiddenLayers/hiddenLayer1/Activation/relu'
 * '<S13>'  : 'state_machine/Chart/r2d/simfcn/RegressionNeuralNetwork Predict/getScore/hiddenLayers/hiddenLayer1/Activation/relu/processNan'
 * '<S14>'  : 'state_machine/Chart/r2d/simfcn/RegressionNeuralNetwork Predict/getScore/hiddenLayers/hiddenLayer1/Activation/relu/processNan/nonfiniteSupported'
 * '<S15>'  : 'state_machine/Chart/r2d/simfcn/RegressionNeuralNetwork Predict/processNanResponse/nonfiniteSupported'
 * '<S16>'  : 'state_machine/Chart/r2d/simfcn/RegressionNeuralNetwork Predict/standardize/passthrough'
 */
#endif                                 /* state_machine_h_ */

/*
 * File trailer for generated code.
 *
 * [EOF]
 */
