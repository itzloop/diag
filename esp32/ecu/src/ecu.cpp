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

void convertToBytes(long int num, uint8_t *arr, int len)
{
    for (int i = 0; i < len; i++)
    {
        uint8_t b = (num >> ((len - 1) - i) * 8) & 0xFF;
        arr[i] = b;
    }
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

void simulate_data(ecu_data *data)
{
    // set engine speed
    convertToBytes(random(0, 16383), data->engineSpeed, 2);
    // convertToBytes(420, data->engineSpeed, 2);
    convertToBytes(random(0, 100), data->throttle, 1);
    // convertToBytes(85, data->throttle, 1);
    convertToBytes(random(0, 255), data->vehicleSpeed, 1);
    // convertToBytes(69, data->vehicleSpeed, 1);
}

ECU::ECU()
{
}

void ECU::init(HardwareSerial &serial, uint8_t tx, uint8_t rx)
{
    this->_serial = &serial;
    this->rx_pin = rx;
    this->tx_pin = tx;

    this->_prevtime = 0;
    this->_has_initialized = false;
    this->state = SLEEP;

    convertToBytes(0, this->data.engineSpeed, 2);
    convertToBytes(0, this->data.throttle, 1);
    convertToBytes(0, this->data.vehicleSpeed, 1);
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
    // this->_serial->println("reached here...");
    // handle init message
    // we need the received message to be {0xC1, 0x33, 0xF1, 0x81, 0x66};
    // DebugPrintln("reading 5 bytes...");
    // delay(1000);

    while (!this->_serial->available())
    {
    }

    uint8_t message[5] = {};
    this->_serial->readBytes(message, 5);

    // printhex(message, 5);

    // DebugPrintln("received init message...");
    // DebugPrintln("calculating checksum");
    // DebugPrintln(calcsum(message, 4));
    // calc checksum
    if (calcsum(message, 4) != 0x66)
    {
        // DebugPrintln("failed to verify checksum");
        uint8_t failure_message[] = {0x83, 0xf1, 0x01, 0x7f, 0xe9, 0x8f};
        this->_serial->write(failure_message, 6);
        this->state = SLEEP;
        return false;
    }

    // DebugPrintln("checksum verification was successful");

    uint8_t success_message[] = {0x83, 0xf1, 0x01, 0xc1, 0xe9, 0x8f};
    this->_serial->write(success_message, 6);
    // DebugPrintln("sent success message");

    this->state = IDLE;

    // DebugPrintln("begin receiving requests...");

    this->_has_initialized = true;
    return true;
}

bool ECU::initialized()
{
    return this->_has_initialized;
}

void ECU::loop()
{
    size_t bytesRead;

    if (!this->_has_initialized)
    {
        return;
    }

    // change values
    simulate_data(&this->data);

    if (this->_serial->available())
    {
        bytesRead = this->_serial->readBytes(this->buffer, 6);
        if (bytesRead == 6)
        {
            // handle message
            uint8_t recvSum = this->buffer[5];
            uint8_t sum = calcsum(this->buffer, 5);
            if (recvSum != sum)
            {
                // TODO send error message
                return;
            }

            // get pid and service
            uint8_t service = this->buffer[3];
            uint8_t pid = this->buffer[4];

            if (!this->supportService(service))
            {
                // TODO send error message
                return;
            }

            if (!this->supportPid(pid))
            {
                // TODO send error message
                return;
            }

            switch (pid)
            {
            case 0x0C: // Engine speed
                send_reponse(this->data.engineSpeed, 2);
                break;
            case 0x0D: // Vehicle speed
                send_reponse(this->data.vehicleSpeed, 1);
                break;
            case 0x11: // Throttle position
                send_reponse(this->data.throttle, 1);
                break;
            default:
                // if it's a request to get supported pids answer with the correct one
                if (pid % 32 == 0)
                {

                    long int supportedList[7] = {
                        SUPPORTED_PID_01_TO_20,
                        SUPPORTED_PID_21_TO_40,
                        SUPPORTED_PID_41_TO_60,
                        SUPPORTED_PID_61_TO_80,
                        SUPPORTED_PID_81_TO_A0,
                        SUPPORTED_PID_A1_TO_C0,
                        SUPPORTED_PID_C1_TO_E0};
                    uint8_t idx = pid / 32;
                    uint8_t data[4] = {0};
                    convertToBytes(supportedList[idx], data, 4);

                    send_reponse(data, 4);
                }
                break;
            }
        }
    }

    return;
}

void ECU::send_reponse(uint8_t *data, int datalen) // 0x55 1
{
    //   raw request: {0xc2, 0x33, 0xf1, 0x01, 0x0d, 0xf4}
    //   returns       0x83  0xf1  0x11  0x41  0xd  0x0  0xd3

    //   raw request: {0xc2, 0x33, 0xf1, 0x01, 0x0c, 0xf3}
    //   returns       0x84, 0xf1, 0x11, 0x41, 0x0c, 0x0c, 0x4c, 0x2b, 0xf3
    int packetlen = 4 + datalen + 1; // 6
    uint8_t response[4 + datalen + 1] = {0};
    response[0] = (0b1 << 7) | datalen; // this is the length
    response[1] = this->buffer[2];
    response[2] = 0x11;
    response[3] = 0x41;

    // set the actual data
    memcpy(&response[4], data, datalen);

    response[packetlen - 1] = calcsum(response, 8);

    while (!this->_serial->availableForWrite())
    {
    }
    this->_serial->write(response, packetlen);
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
    long int supportedList[7] = {
        SUPPORTED_PID_01_TO_20,
        SUPPORTED_PID_21_TO_40,
        SUPPORTED_PID_41_TO_60,
        SUPPORTED_PID_61_TO_80,
        SUPPORTED_PID_81_TO_A0,
        SUPPORTED_PID_A1_TO_C0,
        SUPPORTED_PID_C1_TO_E0};

    if (pid % 32 == 0)
        return true;

    if (pid > 0xE0)
        return false;

    uint8_t idx = pid / 32;
    long int num = supportedList[idx];

    return ((num >> (32 - (pid % 32))) & 0x1) == 1;
}