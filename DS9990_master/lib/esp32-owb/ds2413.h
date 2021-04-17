#ifndef DS2413_H
#define DS2413_H

#include "owb.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DS2413_FAMILY_CODE 0x3A

/**
 * @brief Success and error codes.
 */
typedef enum
{
    DS2413_ERROR_UNKNOWN = -1,  ///< An unknown error occurred, or the value was not set
    DS2413_OK = 0,        ///< Success
    DS2413_ERROR_DEVICE,  ///< A device error occurred
    DS2413_ERROR_CRC,     ///< A CRC error occurred
    DS2413_ERROR_OWB,     ///< A One Wire Bus error occurred
    DS2413_ERROR_NULL,    ///< A parameter or value is NULL
} DS2413_ERROR;

/**
 * @brief Structure containing information related to a single DS2413 device connected
 * via a 1-Wire bus.
 */
typedef struct
{
    bool init;                     ///< True if struct has been initialised, otherwise false
    bool solo;                     ///< True if device is intended to be the only one connected to the bus, otherwise false
    bool use_crc;                  ///< True if CRC checks are to be used when retrieving information from a device on the bus
    const OneWireBus * bus;        ///< Pointer to 1-Wire bus information relevant to this device
    OneWireBus_ROMCode rom_code;   ///< The ROM code used to address this device on the bus
} DS2413_Info;

DS2413_Info * ds2413_malloc(void);

void ds2413_free(DS2413_Info ** ds2413_info);

void ds2413_init(DS2413_Info * ds2413_info, const OneWireBus * bus, OneWireBus_ROMCode rom_code);

void ds2413_use_crc(DS2413_Info * ds2413_info, bool use_crc);

OneWireBus_ROMCode ds2413_read_rom(DS2413_Info * ds2413_info);

DS2413_ERROR ds2413_read_pio(const DS2413_Info * ds2413_info, uint8_t * value);

DS2413_ERROR ds2413_write_pio(const DS2413_Info * ds2413_info, uint8_t value);

#ifdef __cplusplus
}
#endif

#endif  // DS2413_H
