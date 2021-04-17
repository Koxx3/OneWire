#include "DS2413.h"

DS2413::DS2413(uint8_t ID1, uint8_t ID2, uint8_t ID3, uint8_t ID4, uint8_t ID5, uint8_t ID6, uint8_t ID7) : OneWireItem(ID1, ID2, ID3, ID4, ID5, ID6, ID7)
{
    pin_state[0] = false;
    pin_latch[0] = false;
    pin_state[1] = false;
    pin_latch[1] = false;
}

void DS2413::duty(OneWireHub *const hub)
{
    uint8_t cmd, data, datainv;

    if (hub->recv(&cmd))
        return;

    switch (cmd)
    {
    case 0x5A: // PIO ACCESS WRITE

        if (hub->recv(&data))
            return;
        if (hub->recv(&datainv))
            return;
        if (data == ((uint8_t)~datainv))
        {
            setPinLatch(0, (data & static_cast<uint8_t>(0x01)) != 0); // A
            setPinLatch(1, (data & static_cast<uint8_t>(0x02)) != 0); // B

            // send success reply
            uint8_t success = 0xAA;
            hub->send(&success);
        }
        else
        {
            printf("error : data and datainv not matching = %d / %d\n", data, datainv);
            return;
        }
        break;

    case 0xF5: // PIO ACCESS READ

        data = 0;

        if (pin_state[0])
            data |= static_cast<uint8_t>(0x01);
        if (!pin_latch[0])
            data |= static_cast<uint8_t>(0x02);
        if (pin_state[1])
            data |= static_cast<uint8_t>(0x04);
        if (!pin_latch[1])
            data |= static_cast<uint8_t>(0x08);

        data = data | (((uint8_t)~data) << 4);
        printf("read = %02x\n", data);

        if (hub->send(&data))
            return;
        break;

    default:
        hub->raiseSlaveError(cmd);
    }
}
