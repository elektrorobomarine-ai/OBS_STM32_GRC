#include "app.h"

#include "board_adc.h"
#include "board_analog.h"
#include "board_depth.h"
#include "board_imu.h"
#include "board_relay.h"

#include "obs_network.h"
#include "obs_state.h"
#include "obs_time.h"
#include "obs_usb_logger.h"

bool app_init(void)
{
    obs_time_init();
    obs_state_init();
    board_relay_init();

    if (!board_adc_init()) {
        return false;
    }

    if (!board_imu_init()) {
        return false;
    }

    /*
     * Depth sensor initialization failure is non-fatal at startup.
     * The sensor task will continue attempting reads and expose valid=false.
     */
    (void)board_depth_init();

    if (!board_analog_init()) {
        return false;
    }

    if (!obs_network_init()) {
        return false;
    }

    if (!obs_usb_logger_init()) {
        return false;
    }

    board_imu_start_rx();

    return true;
}

bool app_start(void)
{
    return board_adc_start();
}
