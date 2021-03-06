#include "obd.h"

void printhex(uint8_t *b, uint8_t len)
{
    char hex[] = "0123456789ABCDEF";
    char result[len * 2];
    for (uint8_t i = 0, j = 0; i < len; i++, j += 2)
    {
        uint8_t ch = b[i];
        result[j] = hex[(ch >> 4) & 0xF];
        result[j + 1] = hex[(ch)&0xF];
    }

    Serial.println(result);
}

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

bool OBD2::init(HardwareSerial &serial, uint8_t tx, uint8_t rx)
{
    this->_serial = &serial;
    this->rx = rx;
    this->tx = tx;

#ifdef ECU_WAKEUP_INIT

    this->_serial->end();
    pinMode(this->tx, OUTPUT);

    Serial.println("sending high for 3 seconds");
    digitalWrite(this->tx, LOW);
    delay(3000);
    Serial.println("sending low for 25 ms");
    digitalWrite(this->tx, HIGH);
    delay(25);
    digitalWrite(this->tx, LOW);
    Serial.println("sending high for 25 ms");
    delay(25);
    return false;
#endif
    this->_serial->begin(10400);

    // send init message to ecu
    Serial.println("sending init message to ecu...");
    uint8_t message[5] = {0xC1, 0x33, 0xF1, 0x81, 0x66};
    this->_serial->write(message, 5);

    // wait for init response
    Serial.println("wait for init response");
    uint8_t response[6] = {0};
    int bytes_read = 0;
    while (!this->_serial->available())
    {
    }
    bytes_read = this->_serial->readBytes(response, 6);

    Serial.println("got init response:");

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
    uint8_t request[reqlen + 1] = {0};
    // now we modify the header, the payload is the request_len - 3 header bytes
    request[0] = (0b11 << 6) | (reqlen - 3);
    request[1] = 0x33;
    request[2] = 0xf1;
    request[3] = mode;
    request[4] = pid;
    request[5] = calcsum(request, 5);

    while (!this->_serial->availableForWrite())
    {
    }

    this->_serial->write(request, 6);
    while (!this->_serial->available())
    {
    }

    // calculate the length
    uint8_t d1;
    this->_serial->readBytes(&d1, 1);
    uint8_t responselen = d1 & 0b111111;
    uint8_t response[4 + responselen + 1] = {0}; // 6
    response[0] = d1;
    uint8_t readbytes = this->_serial->readBytes(&response[1], 3 + responselen + 1);
    if (readbytes != 3 + responselen + 1)
    {
        // TODO handle error
        Serial.println("error readbytes != responselen - 1");
        return;
    }

    uint8_t sum = calcsum(response, 4 + responselen);

    if (sum != response[4 + responselen])
    {
        Serial.printf("error sum(%d) != response[4 + responselen](%d)\n", sum, response[4 + responselen]);
        return;
    }
    memset(this->buffer, 0, BUFFER_SIZE);
    memcpy(this->buffer, &response[4], responselen);
}

String OBD2::humanReadable(uint8_t pid, uint8_t mode)
{
    if (mode != 0x01)
        return "";

    char buf[128];
    ;
    switch (pid)
    {
    case 0x0C:
        sprintf(buf, "%.2f", (256 * this->buffer[0] + this->buffer[1]) / (float)4);
        break;
    case 0x0D:
        sprintf(buf, "%d", this->buffer[0]);
        break;
    case 0x11:
        // Serial.printf("throttle is %d\n", this->buffer[0]);
        sprintf(buf, "%.2f", ((float)100 / 255) * this->buffer[0]);
        break;

    default:
        Serial.println("wrong pid");
        return "";
    }

    // Serial.println(buf);
    return String(buf);
}
