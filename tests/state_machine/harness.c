/* One step of state_machine_step. Inputs: the 7 real_T inports and the 2 chart-state bytes,
   treated as free inputs (one step from an arbitrary valid chart state). */
#include "hux.h"
#include "state_machine.h"

int main(void) {
  state_machine_U.brake = __VERIFIER_nondet_double();
  state_machine_U.accellerator = __VERIFIER_nondet_double();
  state_machine_U.button = __VERIFIER_nondet_double();
  state_machine_U.shutdown_closed = __VERIFIER_nondet_double();
  state_machine_U.battery_voltage[0] = __VERIFIER_nondet_double();
  state_machine_U.battery_voltage[1] = __VERIFIER_nondet_double();
  state_machine_U.battery_voltage[2] = __VERIFIER_nondet_double();
  state_machine_DW.is_active_c3_state_machine = __VERIFIER_nondet_uchar();
  state_machine_DW.is_c3_state_machine = __VERIFIER_nondet_uchar();
  __VERIFIER_assume(state_machine_DW.is_active_c3_state_machine <= 1);
  __VERIFIER_assume(state_machine_DW.is_c3_state_machine >= 1 &&
                    state_machine_DW.is_c3_state_machine <= 4);

  HUX_RECORD_FLOAT("state_machine_U.brake", state_machine_U.brake);
  HUX_RECORD_FLOAT("state_machine_U.accellerator", state_machine_U.accellerator);
  HUX_RECORD_FLOAT("state_machine_U.button", state_machine_U.button);
  HUX_RECORD_FLOAT("state_machine_U.shutdown_closed", state_machine_U.shutdown_closed);
  HUX_RECORD_FLOAT("state_machine_U.battery_voltage[0]", state_machine_U.battery_voltage[0]);
  HUX_RECORD_FLOAT("state_machine_U.battery_voltage[1]", state_machine_U.battery_voltage[1]);
  HUX_RECORD_FLOAT("state_machine_U.battery_voltage[2]", state_machine_U.battery_voltage[2]);
  HUX_RECORD_UINT("state_machine_DW.is_active_c3_state_machine",
                  state_machine_DW.is_active_c3_state_machine);
  HUX_RECORD_UINT("state_machine_DW.is_c3_state_machine", state_machine_DW.is_c3_state_machine);

  HUX_START();
  state_machine_step();
  HUX_CHECK_BUCKET();
  return 0;
}
