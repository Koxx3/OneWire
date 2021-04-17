// 1Kbit 1-Wire EEPROM, Add Only Memory
// works, writing could not be tested (DS9490 does not support hi-voltage mode and complains)
// native bus-features: none

#ifndef ONEWIRE_DS9990_H
#define ONEWIRE_DS9990_H

#include "OneWireItem.h"

class DS9990 : public OneWireItem
{
private:
    static constexpr uint8_t MEM_SIZE{8}; // bytes

    uint8_t memory[MEM_SIZE]; // 4 pages of 32 bytes

public:
    static constexpr uint8_t family_code = 0x09; // the DS9990

    DS9990(uint8_t ID1, uint8_t ID2, uint8_t ID3, uint8_t ID4, uint8_t ID5, uint8_t ID6, uint8_t ID7);

    void duty(OneWireHub *hub) final;

    void clearMemory(void);
    uint8_t getCRC(uint8_t size);

    bool writeMemory(const uint8_t *source, uint8_t length, uint8_t position = 0);
    bool readMemory(uint8_t *destination, uint8_t length, uint8_t position = 0) const;
};

#endif
