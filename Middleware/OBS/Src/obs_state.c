#include "obs_state.h"

#include "cmsis_os2.h"

#include <string.h>

static obs_state_t state;
static osMutexId_t state_mutex;

static void state_lock(void)
{
    if (state_mutex != NULL) {
        (void)osMutexAcquire(
            state_mutex,
            osWaitForever);
    }
}

static void state_unlock(void)
{
    if (state_mutex != NULL) {
        (void)osMutexRelease(state_mutex);
    }
}

void obs_state_init(void)
{
    memset(&state, 0, sizeof(state));
    state_mutex = osMutexNew(NULL);
}

void obs_state_set_imu(board_imu_data_t value)
{
    state_lock();
    state.imu = value;
    state_unlock();
}

void obs_state_set_depth(board_depth_data_t value)
{
    state_lock();
    state.depth = value;
    state_unlock();
}

void obs_state_set_analog(board_analog_data_t value)
{
    state_lock();
    state.analog = value;
    state_unlock();
}

void obs_state_set_usb(
    bool mounted,
    bool logging)
{
    state_lock();

    state.usb_mounted = mounted;
    state.usb_logging = logging;

    state_unlock();
}

obs_state_t obs_state_get(void)
{
    obs_state_t snapshot;

    state_lock();
    snapshot = state;
    state_unlock();

    return snapshot;
}
