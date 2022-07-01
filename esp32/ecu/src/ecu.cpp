#include "ecu.h"

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

ECU::ECU() {}

void ECU::init(HardwareSerial &serial, uint8_t tx, uint8_t rx)
{
    this->_serial = &serial;
    this->rx_pin = rx;
    this->tx_pin = tx;

    this->_prevtime = 0;
    this->_has_initialized = false;
    this->state = SLEEP;
}

bool ECU::wakeup()
{

#ifdef ECU_WAKEUP_INIT
    int prev = 0, val;
    this->state = FIRST_INIT;

    for (;;)
    {
        switch (this->state)
        {
        case FIRST_INIT:
            val = wakeup_state_helper(&prev, 300, 1, SECOND_INIT);

            break;

        case SECOND_INIT:
            wakeup_state_helper(&prev, 25, 0, THIRD_INIT);
            break;

        case THIRD_INIT:
            wakeup_state_helper(&prev, 25, 1, MESSAGE_INIT);
            goto after_loop;

        default:
            break;
        }

        delay(5);
    }
#endif

after_loop:
    // Serial.printf("prevtime: %d, state: %d, val: %d\n", prev, this->state, val);
    this->_serial->begin(10400);
    this->_serial->println("reached here...");
    // handle init message
    // we need the received message to be {0xC1, 0x33, 0xF1, 0x81, 0x66};
    // DebugPrintln("reading 5 bytes...");
    delay(1000);

    while (Serial.available())
    {
        uint8_t message[5] = {};
        this->_serial->readBytes(message, 5);

        printhex(message, 5);

        DebugPrintln("received init message...");
        DebugPrintln("calculating checksum");
        DebugPrintln(calcsum(message, 4));
        // calc checksum
        if (calcsum(message, 4) != 0x66)
        {
            DebugPrintln("failed to verify checksum");
            uint8_t failure_message[] = {0x83, 0xf1, 0x01, 0x7f, 0xe9, 0x8f};
            this->_serial->write(failure_message, 6);
            this->state = SLEEP;
            return false;
        }

        DebugPrintln("checksum verification was successful");

        uint8_t success_message[] = {0x83, 0xf1, 0x01, 0xc1, 0xe9, 0x8f};
        this->_serial->write(success_message, 6);
        DebugPrintln("sent success message");

        this->state = IDLE;

        DebugPrintln("begin receiving requests...");
    }

    return true;
}

bool ECU::loop()
{

    if (this->_serial->available())
    {
    }

    return false;
}

int ECU::wakeup_state_helper(int *prevtime, int duration, int valtocheck, enum ECU_STATE state_to_go)
{
    int val = digitalRead(this->rx_pin);
    if (val != valtocheck && (millis() - *prevtime < duration))
    {
        // TODO error
    }
    else if (val == valtocheck && (millis() - *prevtime >= duration))
    {
        // first stage is ok
        this->state = state_to_go;
        *prevtime = millis();
    }

    return val;
}

bool ECU::supportService(uint8_t service)
{
    return service == SUPPORTED_SERVICE;
}
bool ECU::supportPid(uint8_t pid)
{
    if (pid % 32 == 0)
        return true;

    if (pid > 0xE0)
        return false;

    uint8_t idx = pid / 32;
    int num = this->supportedList[idx];

    return ((num >> (32 - (pid % 32))) & 0x1) == 1;
}

void ECU::writeInterval()
{
    this->_serial->write("hello from ECU");
}