/*
 * MIT License
 *
 * Copyright (c) 2017-2019 David Antliff
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file ds2413.c
 *
 * Resolution is cached in the DS2413_Info object to avoid querying the hardware
 * every time a temperature conversion is required. However this can result in the
 * cached value becoming inconsistent with the hardware value, so care must be taken.
 *
 */

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_log.h"

#include "ds2413.h"
#include "owb.h"

static const char *TAG = "ds2413";
static const int T_CONV = 750; // maximum conversion time at 12-bit resolution in milliseconds

// Function commands
#define DS2413_FUNCTION_PIO_WRITE 0x5A
#define DS2413_FUNCTION_PIO_READ 0xF5

/// @cond ignore
typedef struct
{
    uint8_t temperature[2]; // [0] is LSB, [1] is MSB
    uint8_t trigger_high;
    uint8_t trigger_low;
    uint8_t configuration;
    uint8_t reserved[3];
    uint8_t crc;
} __attribute__((packed)) Scratchpad;
/// @endcond ignore

static void _init(DS2413_Info *ds2413_info, const OneWireBus *bus)
{
    if (ds2413_info != NULL)
    {
        ds2413_info->bus = bus;
        memset(&ds2413_info->rom_code, 0, sizeof(ds2413_info->rom_code));
        ds2413_info->use_crc = false;
        ds2413_info->solo = false; // assume multiple devices unless told otherwise
        ds2413_info->init = true;
    }
    else
    {
        ESP_LOGE(TAG, "ds2413_info is NULL");
    }
}

static bool _is_init(const DS2413_Info *ds2413_info)
{
    bool ok = false;
    if (ds2413_info != NULL)
    {
        if (ds2413_info->init)
        {
            // OK
            ok = true;
        }
        else
        {
            ESP_LOGE(TAG, "ds2413_info is not initialised");
        }
    }
    else
    {
        ESP_LOGE(TAG, "ds2413_info is NULL");
    }
    return ok;
}

static bool _address_device(const DS2413_Info *ds2413_info)
{
    bool present = false;
    if (_is_init(ds2413_info))
    {
        owb_reset(ds2413_info->bus, &present);
        if (present)
        {
            if (ds2413_info->solo)
            {
                // if there's only one device on the bus, we can skip
                // sending the ROM code and instruct it directly
                owb_write_byte(ds2413_info->bus, OWB_ROM_SKIP);
            }
            else
            {
                // if there are multiple devices on the bus, a Match ROM command
                // must be issued to address a specific slave
                owb_write_byte(ds2413_info->bus, OWB_ROM_MATCH);
                owb_write_rom_code(ds2413_info->bus, ds2413_info->rom_code);
            }
        }
        else
        {
            ESP_LOGE(TAG, "ds2413 device not responding");
        }
    }
    return present;
}

static DS2413_ERROR _read_pio(const DS2413_Info *ds2413_info, uint8_t *value)
{
    // If CRC is enabled, regardless of count, read the entire scratchpad and verify the CRC,
    // otherwise read up to the scratchpad size, or count, whichever is smaller.

    DS2413_ERROR err = DS2413_ERROR_UNKNOWN;

    if (_address_device(ds2413_info))
    {
        // read scratchpad
        if (owb_write_byte(ds2413_info->bus, DS2413_FUNCTION_PIO_READ) == OWB_STATUS_OK)
        {
            if (owb_read_byte(ds2413_info->bus, (uint8_t *)value) == OWB_STATUS_OK)
            {
                err = DS2413_OK;
                // Without CRC, or partial read:
                ESP_LOGD(TAG, "No CRC check");
                bool is_present = false;
                owb_reset(ds2413_info->bus, &is_present); // terminate early
            }
            else
            {
                ESP_LOGE(TAG, "owb_read_bytes failed");
                err = DS2413_ERROR_OWB;
            }
        }
        else
        {
            ESP_LOGE(TAG, "owb_write_byte failed");
            err = DS2413_ERROR_OWB;
        }
    }
    else
    {

        ESP_LOGE(TAG, "_address_device failed");
        err = DS2413_ERROR_OWB;
    }
    return err;
}

static bool _write_pio(const DS2413_Info *ds2413_info, const uint8_t value, bool verify)
{
    bool result = false;
    // Only bytes 2, 3 and 4 (trigger and configuration) can be written.
    // All three bytes MUST be written before the next reset to avoid corruption.
    if (_is_init(ds2413_info))
    {
        if (_address_device(ds2413_info))
        {
            //printf("pio write 1 byte: %02x\n", value);

            owb_write_byte(ds2413_info->bus, DS2413_FUNCTION_PIO_WRITE);
            owb_write_byte(ds2413_info->bus, value);
            owb_write_byte(ds2413_info->bus, ((uint8_t)~value));
            result = true;

            if (verify)
            {
                uint8_t ret = 0xbb;
                if (owb_read_byte(ds2413_info->bus, (uint8_t *)&ret) == OWB_STATUS_OK)
                {
                    //printf("received : %02x\n", ret);
                    //printf("inverted sent : %02x\n", (uint8_t)~value);

                    if (((uint8_t)ret) == 0xAA)
                    {
                        //printf("ret 0xAA => OK\n", ret);

                        uint8_t finish = 0xff;
                        owb_write_byte(ds2413_info->bus, finish);

                        bool is_present = true;
                        owb_reset(ds2413_info->bus, &is_present); // terminate early
                    }
                    else
                    {
                        printf("ret %d => KO !\n", ret);

                        bool is_present = false;
                        owb_reset(ds2413_info->bus, &is_present); // terminate early

                        result = false;
                    }
                }
                else
                {
                    ESP_LOGE(TAG, "owb_read_bytes failed");
                    result = false;
                }
            }
        }
    }
    return result;
}

// Public API

DS2413_Info *ds2413_malloc(void)
{
    DS2413_Info *ds2413_info = malloc(sizeof(*ds2413_info));
    if (ds2413_info != NULL)
    {
        memset(ds2413_info, 0, sizeof(*ds2413_info));
        ESP_LOGD(TAG, "malloc %p", ds2413_info);
    }
    else
    {
        ESP_LOGE(TAG, "malloc failed");
    }

    return ds2413_info;
}

void ds2413_free(DS2413_Info **ds2413_info)
{
    if (ds2413_info != NULL && (*ds2413_info != NULL))
    {
        ESP_LOGD(TAG, "free %p", *ds2413_info);
        free(*ds2413_info);
        *ds2413_info = NULL;
    }
}

void ds2413_init(DS2413_Info *ds2413_info, const OneWireBus *bus, OneWireBus_ROMCode rom_code)
{
    if (ds2413_info != NULL)
    {
        _init(ds2413_info, bus);
        ds2413_info->rom_code = rom_code;
    }
    else
    {
        ESP_LOGE(TAG, "ds2413_info is NULL");
    }
}

void ds2413_use_crc(DS2413_Info *ds2413_info, bool use_crc)
{
    if (_is_init(ds2413_info))
    {
        ds2413_info->use_crc = use_crc;
        ESP_LOGD(TAG, "use_crc %d", ds2413_info->use_crc);
    }
}

DS2413_ERROR ds2413_read_pio(const DS2413_Info *ds2413_info, uint8_t *value)
{
    DS2413_ERROR err = DS2413_ERROR_UNKNOWN;
    if (_is_init(ds2413_info))
    {
        // read scratchpad up to and including configuration register
        _read_pio(ds2413_info, value);
    }
    return err;
}

DS2413_ERROR ds2413_write_pio(const DS2413_Info *ds2413_info, uint8_t value)
{
    bool result = false;
    DS2413_ERROR err = DS2413_ERROR_UNKNOWN;
    if (_is_init(ds2413_info))
    {
        result = _write_pio(ds2413_info, value, true);
    }

    if (result == true)
        err = DS2413_OK;

    return err;
}
