#include "obd.h"

uint8_t calcsum(uint8_t *arr, int len)
{
    if (arr == NULL)
    {
        return -1;
    }

    uint8_t sum = 0;
    for (int i = 0; i < len; i++)
    {
        sum += arr[i];
    }

    return sum;
}

bool OBD2::init(HardwareSerial &serial, uint8_t rx, uint8_t tx)
{
    this->_serial = &serial;
    this->rx = rx;
    this->tx = tx;

    this->_serial->end();
    pinMode(this->tx, OUTPUT);
    digitalWrite(this->tx, HIGH);

    digitalWrite(this->tx, 1);
    delay(3000);
    digitalWrite(this->tx, 0);
    delay(25);
    digitalWrite(this->tx, 1);
    delay(25);

    Serial2.begin(10400);

    // send init message to ecu
    uint8_t message[5] = {0xC1, 0x33, 0xF1, 0x81, 0x66};
    while (this->_serial->availableForWrite())
    {
        this->_serial->write(message, 5);
    }

    // wait for init response
    uint8_t response[6] = {0};
    int bytes_read = 0;
    while (this->_serial->available() && bytes_read != 6)
    {
        bytes_read = this->_serial->readBytes(response, 6);
    }

    if (response[3] == 0xC1)
    {
        Serial.println("connected to ecu =))))");
        return true;
    }

    Serial.println("failed to connect ecu :_(");
    return false;
}

/*
for ISO 14230 KWP:
      First byte lower 6 bits are length, first two bits always 0b11?

      raw request: {0xc2, 0x33, 0xf1, 0x01, 0x0d, 0xf4}
      returns       0x83  0xf1  0x11  0x41  0xd  0x0  0xd3

      raw request: {0xc2, 0x33, 0xf1, 0x01, 0x0c, 0xf3}
      returns       0x84, 0xf1, 0x11, 0x41, 0x0c, 0x0c, 0x4c, 0x2b, 0xf3
*/

void OBD2::getPid(uint8_t pid, uint8_t mode)
{
    uint8_t reqlen = 5;
    uint8_t request[reqlen] = {0};
    // now we modify the header, the payload is the request_len - 3 header bytes
    request[0] = (0b11 << 6) | (reqlen - 3);
    request[1] = 0x33;
    request[2] = 0xf1;
    request[3] = mode;
    request[4] = pid;
    request[5] = calcsum(request, 4);

    while (!this->_serial->availableForWrite())
    {
    }

    this->_serial->write(this->buffer, 5);

    while (!this->_serial->available())
    {
    }

    // uint8_t reponse[retlen] = {0};
    // calculate the length
    uint8_t responselen;
    this->_serial->readBytes(&responselen, 1);

    responselen = responselen & 0b111111;
    uint8_t response[4 + responselen + 1];
    response[0] = responselen;

    uint8_t readbytes = this->_serial->readBytes(response, 3 + responselen + 1);
    if (readbytes != responselen - 1)
    {
        // TODO handle error
        Serial.println("error readbytes != responselen - 1");
        return;
    }

    memset(this->buffer, 0, BUFFER_SIZE);
    uint8_t sum = calcsum(response, responselen - 1);

    if (sum != response[responselen - 1])
    {
        Serial.println("error sum != response[responselen - 1]");
        return;
    }

    memset(this->buffer, 0, BUFFER_SIZE);
    memcpy(this->buffer, &response[4], responselen);
}
