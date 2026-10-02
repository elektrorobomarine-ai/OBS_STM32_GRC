#include "obs_network.h"

#include "app_config.h"
#include "board_adc.h"
#include "board_relay.h"
#include "obs_nmea.h"
#include "obs_state.h"
#include "obs_time.h"
#include "obs_usb_logger.h"

#include "cmsis_os2.h"

#include "lwip/inet.h"
#include "lwip/sockets.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    board_adc_block_t block;
} network_adc_queue_item_t;

static osMessageQueueId_t adc_queue;

static int command_listen_socket = -1;
static int data_listen_socket = -1;

static int command_clients[OBS_TCP_MAX_CLIENTS];
static int data_clients[OBS_TCP_MAX_CLIENTS];

static uint32_t data_sequence;

static void socket_set_nonblocking(int socket_fd)
{
    u_long nonblocking = 1U;

    (void)lwip_ioctl(
        socket_fd,
        FIONBIO,
        &nonblocking);
}

static int create_listen_socket(uint16_t port)
{
    int socket_fd;
    int reuse_address = 1;
    struct sockaddr_in address;

    socket_fd =
        lwip_socket(
            AF_INET,
            SOCK_STREAM,
            0);

    if (socket_fd < 0) {
        return -1;
    }

    (void)lwip_setsockopt(
        socket_fd,
        SOL_SOCKET,
        SO_REUSEADDR,
        &reuse_address,
        sizeof(reuse_address));

    memset(&address, 0, sizeof(address));

    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    if (lwip_bind(
            socket_fd,
            (struct sockaddr *)&address,
            sizeof(address)) < 0) {
        lwip_close(socket_fd);
        return -1;
    }

    if (lwip_listen(socket_fd, 2) < 0) {
        lwip_close(socket_fd);
        return -1;
    }

    socket_set_nonblocking(socket_fd);

    return socket_fd;
}

static void accept_new_client(
    int listen_socket,
    int *client_array)
{
    struct sockaddr_in remote_address;
    socklen_t remote_length = sizeof(remote_address);

    int client_socket =
        lwip_accept(
            listen_socket,
            (struct sockaddr *)&remote_address,
            &remote_length);

    if (client_socket < 0) {
        return;
    }

    socket_set_nonblocking(client_socket);

    for (
        uint32_t index = 0U;
        index < OBS_TCP_MAX_CLIENTS;
        index++) {
        if (client_array[index] < 0) {
            client_array[index] = client_socket;
            return;
        }
    }

    lwip_close(client_socket);
}

static bool socket_send_all(
    int socket_fd,
    const void *buffer,
    size_t length)
{
    const uint8_t *bytes =
        (const uint8_t *)buffer;

    size_t offset = 0U;
    const uint32_t start_tick = HAL_GetTick();

    while (offset < length) {
        int sent =
            lwip_send(
                socket_fd,
                bytes + offset,
                length - offset,
                0);

        if (sent > 0) {
            offset += (size_t)sent;
            continue;
        }

        if (
            sent < 0 &&
            (errno == EWOULDBLOCK ||
             errno == EAGAIN)) {
            if ((HAL_GetTick() - start_tick) > 100U) {
                return false;
            }

            osDelay(1U);
            continue;
        }

        return false;
    }

    return true;
}

static void close_client(int *socket_fd)
{
    if (*socket_fd >= 0) {
        lwip_close(*socket_fd);
        *socket_fd = -1;
    }
}

static void transmit_command_sentence(const char *body)
{
    char sentence[128];

    const size_t length =
        obs_nmea_build(
            sentence,
            sizeof(sentence),
            body);

    if (length == 0U) {
        return;
    }

    for (
        uint32_t index = 0U;
        index < OBS_TCP_MAX_CLIENTS;
        index++) {
        if (
            command_clients[index] >= 0 &&
            !socket_send_all(
                command_clients[index],
                sentence,
                length)) {
            close_client(&command_clients[index]);
        }
    }
}

static void handle_rmcmd(int command)
{
    switch (command) {
    case 1:
        board_relay_set(BOARD_RELAY_1, true);
        break;

    case 5:
        board_relay_set(BOARD_RELAY_1, false);
        break;

    case 2:
        board_relay_set(BOARD_RELAY_2, true);
        break;

    case 6:
        board_relay_set(BOARD_RELAY_2, false);
        break;

    case 61:
        (void)obs_usb_logger_start();
        break;

    case 62:
        obs_usb_logger_stop();
        break;

    default:
        break;
    }
}

static void handle_command_body(char *body)
{
    if (strncmp(body, "RMCMD,", 6U) == 0) {
        handle_rmcmd(atoi(body + 6));
        return;
    }

    if (strncmp(body, "TIME1,", 6U) == 0) {
        unsigned ms;
        unsigned year;
        unsigned month;
        unsigned weekday;
        unsigned date;
        unsigned hour;
        unsigned minute;
        unsigned second;

        int fields =
            sscanf(
                body + 6,
                "%u,%u,%u,%u,%u,%u,%u,%u",
                &ms,
                &year,
                &month,
                &weekday,
                &date,
                &hour,
                &minute,
                &second);

        if (fields == 8) {
            obs_utc_t utc_time = {
                .year = (uint16_t)year,
                .month = (uint8_t)month,
                .date = (uint8_t)date,
                .weekday = (uint8_t)weekday,
                .hour = (uint8_t)hour,
                .minute = (uint8_t)minute,
                .second = (uint8_t)second,
                .ms = (uint16_t)ms,
            };

            (void)obs_time_set_utc(&utc_time);
        }

        return;
    }

    if (strncmp(body, "XCFGX", 5U) == 0) {
        char response[100];

        (void)snprintf(
            response,
            sizeof(response),
            "XFWVR,%s,%s",
            OBS_FW_VERSION_STR,
            OBS_FW_TITLE);

        transmit_command_sentence(response);
    }
}

static void receive_command_data(void)
{
    static char receive_buffer[OBS_TCP_MAX_CLIENTS][256];
    static size_t used_length[OBS_TCP_MAX_CLIENTS];

    for (
        uint32_t client_index = 0U;
        client_index < OBS_TCP_MAX_CLIENTS;
        client_index++) {
        if (command_clients[client_index] < 0) {
            continue;
        }

        int received =
            lwip_recv(
                command_clients[client_index],
                receive_buffer[client_index] +
                    used_length[client_index],
                sizeof(receive_buffer[client_index]) -
                    1U -
                    used_length[client_index],
                0);

        if (received == 0) {
            close_client(
                &command_clients[client_index]);

            used_length[client_index] = 0U;
            continue;
        }

        if (received < 0) {
            if (
                errno != EWOULDBLOCK &&
                errno != EAGAIN) {
                close_client(
                    &command_clients[client_index]);

                used_length[client_index] = 0U;
            }

            continue;
        }

        used_length[client_index] +=
            (size_t)received;

        receive_buffer[client_index]
                      [used_length[client_index]] = '\0';

        char *line_end;

        while (
            (line_end =
                 strstr(
                     receive_buffer[client_index],
                     "\r\n")) != NULL) {
            char *body = NULL;

            *line_end = '\0';

            if (obs_nmea_validate(
                    receive_buffer[client_index],
                    &body)) {
                handle_command_body(body);
            }

            const size_t consumed =
                (size_t)(
                    line_end -
                    receive_buffer[client_index]) +
                2U;

            memmove(
                receive_buffer[client_index],
                receive_buffer[client_index] +
                    consumed,
                used_length[client_index] -
                    consumed);

            used_length[client_index] -= consumed;

            receive_buffer[client_index]
                          [used_length[client_index]] = '\0';
        }
    }
}

static void transmit_telemetry(void)
{
    static uint32_t fast_tick;
    static uint32_t medium_tick;
    static uint32_t slow_tick;
    static uint32_t counter;

    const uint32_t now = HAL_GetTick();
    const obs_state_t state = obs_state_get();

    char body[120];

    if (
        (now - fast_tick) >=
        OBS_TELEM_FAST_PERIOD_MS) {
        fast_tick = now;

        (void)snprintf(
            body,
            sizeof(body),
            "DEPT0,%ld,%ld,%ld,%lu",
            (long)(state.depth.depth_m * 10.0f),
            (long)(state.depth.rate_mps * 100.0f),
            (long)(state.depth.temperature_c * 100.0f),
            (unsigned long)counter);

        transmit_command_sentence(body);

        (void)snprintf(
            body,
            sizeof(body),
            "AHRS2,%ld,%ld,%ld,%ld,%ld,%ld,0,%lu",
            (long)(state.imu.roll_deg * 10.0f),
            (long)(state.imu.pitch_deg * 10.0f),
            (long)(state.imu.yaw_deg * 10.0f),
            (long)(state.imu.p_dps * 100.0f),
            (long)(state.imu.q_dps * 100.0f),
            (long)(state.imu.r_dps * 100.0f),
            (unsigned long)counter++);

        transmit_command_sentence(body);
    }

    if (
        (now - medium_tick) >=
        OBS_TELEM_MEDIUM_PERIOD_MS) {
        medium_tick = now;

        obs_utc_t utc_time;

        if (obs_time_get_utc(&utc_time)) {
            (void)snprintf(
                body,
                sizeof(body),
                "TIME1,%u,%u,%u,%u,%u,%u,%u,%u",
                utc_time.ms,
                utc_time.year,
                utc_time.month,
                utc_time.weekday,
                utc_time.date,
                utc_time.hour,
                utc_time.minute,
                utc_time.second);

            transmit_command_sentence(body);
        }

        (void)snprintf(
            body,
            sizeof(body),
            "XCHM1,%ld,%ld,0,0,0,0,0,%u,0,0,%lu",
            (long)(state.analog.battery_v * 10.0f),
            (long)(state.analog.leak_v * 10.0f),
            state.analog.leak_alarm ? 1U : 0U,
            (unsigned long)counter);

        transmit_command_sentence(body);
    }

    if (
        (now - slow_tick) >=
        OBS_TELEM_SLOW_PERIOD_MS) {
        board_adc_diag_t adc_diagnostics;

        slow_tick = now;

        board_adc_get_diagnostics(
            &adc_diagnostics);

        (void)snprintf(
            body,
            sizeof(body),
            "XCHM0,0,%u",
            (unsigned)(
                adc_diagnostics.sai_errors != 0U ||
                adc_diagnostics.channel_id_errors != 0U));

        transmit_command_sentence(body);
    }
}

bool obs_network_init(void)
{
    adc_queue =
        osMessageQueueNew(
            8U,
            sizeof(network_adc_queue_item_t),
            NULL);

    for (
        uint32_t index = 0U;
        index < OBS_TCP_MAX_CLIENTS;
        index++) {
        command_clients[index] = -1;
        data_clients[index] = -1;
    }

    return adc_queue != NULL;
}

bool obs_network_submit_adc(
    const board_adc_block_t *block)
{
    network_adc_queue_item_t item;

    if (block == NULL) {
        return false;
    }

    item.block = *block;

    return
        osMessageQueuePut(
            adc_queue,
            &item,
            0U,
            0U) == osOK;
}

void obs_network_task(void *argument)
{
    (void)argument;

    while (
        command_listen_socket < 0 ||
        data_listen_socket < 0) {
        if (command_listen_socket < 0) {
            command_listen_socket =
                create_listen_socket(
                    OBS_TCP_COMMAND_PORT);
        }

        if (data_listen_socket < 0) {
            data_listen_socket =
                create_listen_socket(
                    OBS_TCP_DATA_PORT);
        }

        osDelay(1000U);
    }

    for (;;) {
        accept_new_client(
            command_listen_socket,
            command_clients);

        accept_new_client(
            data_listen_socket,
            data_clients);

        receive_command_data();
        transmit_telemetry();

        network_adc_queue_item_t item;

        if (osMessageQueueGet(
                adc_queue,
                &item,
                NULL,
                1U) == osOK) {
            uint8_t header[12] = {
                'O',
                'B',
                'S',
                ':',
                0U,
                0U,
                0U,
                0U,
                0U,
                0U,
                0U,
                0U,
            };

            const uint32_t payload_length =
                OBS_ADC_BYTES_PER_RECORD;

            memcpy(
                &header[4],
                &data_sequence,
                sizeof(data_sequence));

            memcpy(
                &header[8],
                &payload_length,
                sizeof(payload_length));

            for (
                uint32_t index = 0U;
                index < OBS_TCP_MAX_CLIENTS;
                index++) {
                if (data_clients[index] < 0) {
                    continue;
                }

                bool ok =
                    socket_send_all(
                        data_clients[index],
                        header,
                        sizeof(header));

                if (ok) {
                    ok =
                        socket_send_all(
                            data_clients[index],
                            item.block.raw_words,
                            payload_length);
                }

                if (!ok) {
                    close_client(
                        &data_clients[index]);
                }
            }

            data_sequence++;
        }

        osDelay(1U);
    }
}
