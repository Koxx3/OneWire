#include "DS9990.h"

DS9990::DS9990(uint8_t ID1, uint8_t ID2, uint8_t ID3, uint8_t ID4, uint8_t ID5, uint8_t ID6, uint8_t ID7) : OneWireItem(ID1, ID2, ID3, ID4, ID5, ID6, ID7)
{

    clearMemory();
    memory[0] = 0x00;
    memory[1] = 0x00;
    memory[2] = 0x00;
    memory[3] = 0x00;
    memory[4] = 0x00;
    memory[5] = 0x00;
    memory[6] = 0x00;
    memory[7] = 0x00;
}

uint8_t DS9990::getCRC(uint8_t size)
{
    return crc8(memory, size);
}

void DS9990::duty(OneWireHub *const hub)
{
    uint8_t size_r, size_w, cmd, crc = 0; // Target address, redirected address, command, data, crc
    uint8_t temp[8];
    uint8_t crc_rcv;

    if (hub->recv(&cmd))
        return;

    switch (cmd)
    {
    case 0xF0: // READ MEMORY

        if (hub->recv(&size_r, 1))
            return;

        // hub->send(memory, size);
        for (int i = 0; i < size_r; i++)
        {
            hub->send(memory[i]);
        }

        crc = crc8(memory, size_r, 0);
        hub->send(crc);

        //Serial.printf("read : memory[0] = %02x", memory[0]);
        //Serial.printf(" /// crc: %02x\n", crc);

        break; // datasheet says we should return all 1s, send(255), till reset, nothing to do here, 1s are passive

    case 0x0F: // WRITE MEMORY

        if (hub->recv(&size_w, 1))
            return;

        if (hub->recv(temp, size_w))
            return;

        if (hub->recv(&crc_rcv, 1))
            return;

        crc = crc8(temp, size_w, 0);
        if (crc == crc_rcv)
        {
            memcpy(memory, temp, size_w);

            if (hub->send(&crc))
                return;

            //Serial.printf("write : memory[0] = %02x\n", memory[0]);
            //Serial.printf(" /// crc: %02x\n", crc);
        }

        break;

    case 0xFF: // WRITE & READ MEMORY

        // WRITE
        if (hub->recv(&size_w, 1))
            return;

        if (hub->recv(temp, size_w))
            return;

        if (hub->recv(&crc_rcv, 1))
            return;

        crc = crc8(temp, size_w, 0);
        if (crc == crc_rcv)
        {
            memcpy(memory, temp, size_w);

            if (hub->send(&crc))
                return;
        }

        // READ
        if (hub->recv(&size_r, 1))
            return;

        // hub->send(memory, size);
        for (int i = 0; i < size_r; i++)
        {
            hub->send(memory[i]);
        }

        crc = crc8(memory, size_r, 0);
        hub->send(crc);

        //Serial.printf("write_read : memory[0] = %02x\n", memory[0]);

        break;

    default:

        hub->raiseSlaveError(cmd);
    }
}

void DS9990::clearMemory(void)
{
    memset(memory, static_cast<uint8_t>(0xFF), MEM_SIZE);
}

bool DS9990::writeMemory(const uint8_t *const source, const uint8_t length, const uint8_t position)
{
    if (position >= MEM_SIZE)
        return false;
    memcpy(&memory[position], source, length);

    //Serial.printf("memory[0] = %02x / memory[1] = %02x\n", memory[0], memory[1]);

    return true;
}

bool DS9990::readMemory(uint8_t *const destination, const uint8_t length, const uint8_t position) const
{
    if (position >= MEM_SIZE)
        return false;
    memcpy(destination, &memory[position], length);
    return true;
}
