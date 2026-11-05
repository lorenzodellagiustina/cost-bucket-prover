/* One step of state_machine_step. Inputs: the 7 real_T inports and the 2 chart-state bytes. No
   assumption restricts them: every value of their types is a possible input, including chart-state
   values that are not states of the chart. */
#include "hux.h"
#include "state_machine.h"

int main(void) {
  HUX_INPUT(double, "state_machine_U.brake", state_machine_U.brake);
  HUX_INPUT(double, "state_machine_U.accellerator", state_machine_U.accellerator);
  HUX_INPUT(double, "state_machine_U.button", state_machine_U.button);
  HUX_INPUT(double, "state_machine_U.shutdown_closed", state_machine_U.shutdown_closed);
  HUX_INPUT(double, "state_machine_U.battery_voltage[0]", state_machine_U.battery_voltage[0]);
  HUX_INPUT(double, "state_machine_U.battery_voltage[1]", state_machine_U.battery_voltage[1]);
  HUX_INPUT(double, "state_machine_U.battery_voltage[2]", state_machine_U.battery_voltage[2]);
  HUX_INPUT(uchar, "state_machine_DW.is_active_c3_state_machine",
            state_machine_DW.is_active_c3_state_machine);
  HUX_INPUT(uchar, "state_machine_DW.is_c3_state_machine", state_machine_DW.is_c3_state_machine);

  HUX_START();
  state_machine_step();
  HUX_CHECK_BUCKET();
  return 0;
}
