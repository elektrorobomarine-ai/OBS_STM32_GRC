#include "board_relay.h"
#include "board.h"
#include "app_config.h"

static bool state[2];

static GPIO_PinState pin_state(bool on)
{
#if OBS_RELAY_ACTIVE_HIGH
    return on ? GPIO_PIN_SET : GPIO_PIN_RESET;
#else
    return on ? GPIO_PIN_RESET : GPIO_PIN_SET;
#endif
}

void board_relay_init(void)
{
    board_relay_set(BOARD_RELAY_1, false);
    board_relay_set(BOARD_RELAY_2, false);
}

void board_relay_set(board_relay_id_t id, bool on)
{
    if (id == BOARD_RELAY_1) {
        HAL_GPIO_WritePin(RELAY1_GPIO_Port, RELAY1_Pin, pin_state(on));
        state[0] = on;
    } else if (id == BOARD_RELAY_2) {
        HAL_GPIO_WritePin(RELAY2_GPIO_Port, RELAY2_Pin, pin_state(on));
        state[1] = on;
    }
}

bool board_relay_get(board_relay_id_t id)
{
    return (id == BOARD_RELAY_2) ? state[1] : state[0];
}
