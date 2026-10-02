#include "obs_usb_logger.h"

#include "app_config.h"
#include "obs_state.h"
#include "obs_time.h"

#include "cmsis_os2.h"
#include "fatfs.h"
#include "ff.h"
#include "usb_host.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define OBS_BIN_FILE_MAGIC    0x4F425342UL
#define OBS_BIN_RECORD_MAGIC  0x4F425352UL

#pragma pack(push, 1)

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t sample_rate_hz;
    uint32_t channels;
    uint32_t frames_per_record;
} obs_bin_file_header_t;

typedef struct {
    uint32_t magic;
    uint32_t data_counter;
    uint32_t frame_base_index;

    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
    uint8_t reserved0;

    uint16_t milliseconds;
    uint16_t reserved1;

    uint16_t depth_deci_m;
    int16_t temperature_centi_c;

    int16_t roll_deci_deg;
    int16_t pitch_deci_deg;
    int16_t yaw_deci_deg;

    /*
     * Retained to keep the formal BINARY_LOG_FORMAT.txt record size
     * at 34 bytes.
     */
    uint32_t reserved2;
} obs_bin_record_header_t;

#pragma pack(pop)

_Static_assert(
    sizeof(obs_bin_file_header_t) == 20U,
    "OBS BIN file header must be 20 bytes");

_Static_assert(
    sizeof(obs_bin_record_header_t) == 34U,
    "OBS BIN record header must be 34 bytes");

typedef struct {
    board_adc_block_t block;
} usb_log_queue_item_t;

static FIL log_file;

static bool media_mounted;
static bool logging_active;

static uint32_t data_counter;
static uint32_t file_open_tick;

static osMessageQueueId_t log_queue;

static int16_t scaled_i16(
    float value,
    float scale)
{
    float scaled = value * scale;

    if (scaled > 32767.0f) {
        scaled = 32767.0f;
    }

    if (scaled < -32768.0f) {
        scaled = -32768.0f;
    }

    return (int16_t)lroundf(scaled);
}

static uint16_t scaled_u16(
    float value,
    float scale)
{
    float scaled = value * scale;

    if (scaled > 65535.0f) {
        scaled = 65535.0f;
    }

    if (scaled < 0.0f) {
        scaled = 0.0f;
    }

    return (uint16_t)lroundf(scaled);
}

static bool open_new_log_file(void)
{
    obs_utc_t utc_time;
    char filename[64];
    UINT bytes_written;

    if (
        !media_mounted ||
        !obs_time_get_utc(&utc_time)) {
        return false;
    }

    (void)snprintf(
        filename,
        sizeof(filename),
        "%sOBS_%04u%02u%02u_%02u%02u%02u.bin",
        USBHPath,
        utc_time.year,
        utc_time.month,
        utc_time.date,
        utc_time.hour,
        utc_time.minute,
        utc_time.second);

    if (f_open(
            &log_file,
            filename,
            FA_CREATE_ALWAYS | FA_WRITE) != FR_OK) {
        return false;
    }

    const obs_bin_file_header_t file_header = {
        .magic = OBS_BIN_FILE_MAGIC,
        .version = 1U,
        .sample_rate_hz = OBS_ADC_SAMPLE_RATE_HZ,
        .channels = OBS_ADC_CHANNELS,
        .frames_per_record = OBS_ADC_FRAMES_PER_RECORD,
    };

    if (
        f_write(
            &log_file,
            &file_header,
            sizeof(file_header),
            &bytes_written) != FR_OK ||
        bytes_written != sizeof(file_header)) {
        (void)f_close(&log_file);
        return false;
    }

    logging_active = true;
    file_open_tick = HAL_GetTick();

    obs_state_set_usb(
        true,
        true);

    return true;
}

bool obs_usb_logger_init(void)
{
    log_queue =
        osMessageQueueNew(
            32U,
            sizeof(usb_log_queue_item_t),
            NULL);

    media_mounted = false;
    logging_active = false;
    data_counter = 0U;

    return log_queue != NULL;
}

void obs_usb_logger_service(void)
{
    static uint32_t records_since_sync;

    /*
     * USB Host processing is normally also called by CubeMX-generated
     * application code. Keep this call only if the generated project
     * does not already service MX_USB_HOST_Process() elsewhere.
     */
    if (!media_mounted) {
        MX_USB_HOST_Process();

        if (f_mount(
                &USBHFatFS,
                USBHPath,
                1U) == FR_OK) {
            media_mounted = true;

            obs_state_set_usb(
                true,
                logging_active);
        }
    }

    if (
        logging_active &&
        (HAL_GetTick() - file_open_tick) >=
            (OBS_USB_LOG_ROTATE_SEC * 1000U)) {
        (void)f_sync(&log_file);
        (void)f_close(&log_file);

        logging_active = false;

        (void)open_new_log_file();
    }

    usb_log_queue_item_t item;

    while (
        logging_active &&
        osMessageQueueGet(
            log_queue,
            &item,
            NULL,
            0U) == osOK) {
        const obs_state_t state =
            obs_state_get();

        obs_utc_t utc_time;

        if (!obs_time_get_utc(&utc_time)) {
            memset(
                &utc_time,
                0,
                sizeof(utc_time));
        }

        obs_bin_record_header_t record_header = {0};

        record_header.magic =
            OBS_BIN_RECORD_MAGIC;

        record_header.data_counter =
            data_counter;

        record_header.frame_base_index =
            (uint32_t)(
                item.block.first_frame_index %
                (OBS_ADC_FRAMES_PER_RECORD * 2U));

        record_header.hours = utc_time.hour;
        record_header.minutes = utc_time.minute;
        record_header.seconds = utc_time.second;
        record_header.milliseconds = utc_time.ms;

        record_header.depth_deci_m =
            scaled_u16(
                state.depth.depth_m,
                10.0f);

        record_header.temperature_centi_c =
            scaled_i16(
                state.depth.temperature_c,
                100.0f);

        record_header.roll_deci_deg =
            scaled_i16(
                state.imu.roll_deg,
                10.0f);

        record_header.pitch_deci_deg =
            scaled_i16(
                state.imu.pitch_deg,
                10.0f);

        record_header.yaw_deci_deg =
            scaled_i16(
                state.imu.yaw_deg,
                10.0f);

        UINT bytes_written;

        if (
            f_write(
                &log_file,
                &record_header,
                sizeof(record_header),
                &bytes_written) != FR_OK ||
            bytes_written != sizeof(record_header)) {
            obs_usb_logger_stop();
            break;
        }

        if (
            f_write(
                &log_file,
                item.block.raw_words,
                OBS_ADC_BYTES_PER_RECORD,
                &bytes_written) != FR_OK ||
            bytes_written != OBS_ADC_BYTES_PER_RECORD) {
            obs_usb_logger_stop();
            break;
        }

        data_counter +=
            OBS_ADC_FRAMES_PER_RECORD;

        records_since_sync++;

        if (
            records_since_sync >=
            OBS_USB_SYNC_RECORDS) {
            (void)f_sync(&log_file);
            records_since_sync = 0U;
        }
    }
}

bool obs_usb_logger_start(void)
{
    if (logging_active) {
        return true;
    }

    return open_new_log_file();
}

void obs_usb_logger_stop(void)
{
    if (logging_active) {
        (void)f_sync(&log_file);
        (void)f_close(&log_file);
    }

    logging_active = false;

    obs_state_set_usb(
        media_mounted,
        false);
}

bool obs_usb_logger_submit(
    const board_adc_block_t *block)
{
    usb_log_queue_item_t item;

    if (!logging_active || block == NULL) {
        return false;
    }

    item.block = *block;

    return
        osMessageQueuePut(
            log_queue,
            &item,
            0U,
            0U) == osOK;
}

bool obs_usb_logger_is_active(void)
{
    return logging_active;
}

bool obs_usb_logger_is_mounted(void)
{
    return media_mounted;
}
