#include "obd.h"

bool OBD::init(HardwareSerial &serial, uint8_t *rx, uint8_t *tx)
{
    this->_serial.end();
    pinMode(this->, OUTPUT);
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
    while (this->_serial.availableForWrite())
    {
        this->_serial.write(message, 5);
    }

    // wait for init response
    uint8_t response[6] = {0};
    int bytes_read = 0;
    while (this->_serial.available() && bytes_read != 6)
    {
        bytes_read = this->_serial.readBytes(response, 6);
    }

    if (response[3] == 0xC1)
    {
        Serial.println("connected to ecu =))))");
        return true
    }

    Serial.println("failed to connect ecu :_(");
    return false
}

void OBD::getPid(uint8_t *pid, uint8_t *mode, uint8_t request_length)
{

    uint8_t rbuf[request_length];
    memcpy(rbuf, request, request_len);
    // now we modify the header, the payload is the request_len - 3 header bytes
    rbuf[0] = (0b11 << 6) | (request_len - 3);
    rbuf[1] = 0x33; // second byte should be 0x33
}
