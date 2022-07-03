#include "ecu.h"

ECU *ECU::instance;

// returns 0 if there is data avaiable in serial and 1 if timed out
int avaiableWithTimeout(HardwareSerial &serial, unsigned long timeout)
{
    unsigned long prev = millis();
    for (;;)
    {
        if (serial.available())
            return 0;

        if (millis() - prev >= timeout)
            return 1;
    }
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
    instance = this;
}

void ECU::handle_int()
{
    instance->handleInterrupt();
}

void ECU::handleInterrupt()
{
    Serial.println("fuck");
    if (this->state == SLEEP)
    {
        DebugPrintln("going to first init");
        this->_prevtime = millis();
        this->state = FIRST_INIT;
        return;
    }

    if (this->state == FIRST_INIT && (millis() - this->_prevtime >= 3000))
    {
        DebugPrintln("going to second init");
        this->_prevtime = millis();
        this->state = SECOND_INIT;
        return;
    }

    if (this->state == SECOND_INIT && (millis() - this->_prevtime >= 25))
    {
        DebugPrintln("going to third init");
        this->_prevtime = millis();
        this->state = THIRD_INIT;
        return;
    }

    if (this->state == THIRD_INIT && (millis() - this->_prevtime >= 25))
    {
        DebugPrintln("going to message init");
        this->state = MESSAGE_INIT;
        this->_prevtime = millis();
        return;
    }
}

void ECU::init(HardwareSerial &serial, uint8_t tx, uint8_t rx)
{
    this->_serial = &serial;
    this->_serial->setTimeout(5000);
    this->rx_pin = rx;
    this->tx_pin = tx;

    this->_prevtime = 0;
    this->_has_initialized = false;
    this->state = SLEEP;
    this->_last_message_time = millis();

    convertToBytes(0, this->data.engineSpeed, 2);
    convertToBytes(0, this->data.throttle, 1);
    convertToBytes(0, this->data.vehicleSpeed, 1);
#ifdef ECU_WAKEUP_INIT
    instance = this;
    attachInterrupt(this->rx_pin, handle_int, CHANGE);
#endif
}

bool ECU::wakeup()
{

#ifdef ECU_WAKEUP_INIT

    unsigned long prev = 0;
    int val, prevval = 0;
    this->state = SLEEP;
    Serial.println("initializing with fast init...");

    this->_serial->end();
    pinMode(this->rx_pin, INPUT_PULLDOWN);
    Serial.printf("rx pin: %d, tx pin: %d\n", this->rx_pin, this->tx_pin);
    for (;;)
    {
        val = digitalRead(this->rx_pin);

        // Serial.println(val);
        // if we see one start count

        switch (this->state)
        {
        case SLEEP:
            // if val is changed to 1
            // start couting
            if (val != prevval && val == 1)
            {
                this->state = FIRST_INIT;
                prev = millis();
            }

            break;
        case FIRST_INIT:
            if (val == 1)
            {
                // we need 1 for 3 seconds
                if (millis() - prev >= 2800)
                {
                    this->state = SECOND_INIT;
                }
            }
            else // error
            {
                DebugPrintln("ecu first init failed go back to sleep");
                return false;
            }

            // val = wakeup_state_helper(&prev, 3000, 1, SECOND_INIT);
            // Serial.println(val);

            break;

        case SECOND_INIT:
            if (val == 0)
            {
                // we need 1 for 3 seconds
                if (millis() - prev >= 20)
                {
                    this->state = THIRD_INIT;
                }
            }
            else
            { // error
                DebugPrintln("ecu second init failed go back to sleep");
                // return false;
            }
            // wakeup_state_helper(&prev, 25, 0, THIRD_INIT);
            // Serial.println(val);

            break;

        case THIRD_INIT:

            // wakeup_state_helper(&prev, 25, 1, MESSAGE_INIT);
            // Serial.println("handling second init");
            if (val == 1)
            {
                // we need 1 for 3 seconds
                if (millis() - prev >= 20)
                {
                    this->state = THIRD_INIT;
                }
            }
            else
            { // error
                DebugPrintln("ecu third init failed go back to sleep");
                // return false;
            }
            goto after_loop;

        default:
            break;
        }
    }
#endif

after_loop:
    DebugPrintln("ecu is awake");
    this->state = MESSAGE_INIT;
    // Serial.printf("prevtime: %d, state: %d, val: %d\n", prev, this->state, val);
    this->_serial->begin(10400);
    // this->_serial->println("reached here...");
    // handle init message
    // we need the received message to be {0xC1, 0x33, 0xF1, 0x81, 0x66};
    // DebugPrintln("reading 5 bytes...");
    // delay(1000);

    uint8_t message[5] = {0};
    size_t readlen = this->_serial->readBytes(message, 5);
    if (readlen == 0)
    {
        // timedout
        DebugPrintln("couldn't read init message: timeout");
        return false;
    }

    this->_last_message_time = millis();
    // printhex(message, 5);

    DebugPrintln("received init message...");
    printhex(message, 5);
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

    if (!this->_has_initialized || this->state == SLEEP)
    {
        return;
    }

    // change values
    simulate_data(&this->data);

    if (this->_serial->available())
    {
        this->_last_message_time = millis();
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

    // if we didn't recv a message for 10 seconds
    // go back to sleep
    if (millis() - this->_last_message_time >= 10000)
    {
        this->state = SLEEP;
        this->_has_initialized = false;
        Serial.println("going back to sleep");
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
    uint8_t response[packetlen] = {0};
    response[0] = (0b1 << 7) | datalen; // this is the length
    response[1] = this->buffer[2];
    response[2] = 0x11;
    response[3] = 0x41;

    // set the actual data
    memcpy(&response[4], data, datalen);
    Serial.printf("responsing with %d bytes\n", packetlen);
    uint8_t sum = calcsum(response, packetlen);
    Serial.printf("calculated checksum %d\n", sum);
    response[packetlen - 1] = sum;
    printhex(response, packetlen);

    while (!this->_serial->availableForWrite())
    {
    }
    this->_serial->write(response, packetlen);
}

int ECU::wakeup_state_helper(unsigned long *prevtime, unsigned long duration, int valtocheck, enum ECU_STATE state_to_go)
{
    int val = digitalRead(this->rx_pin);
    Serial.printf("received val %d and checking it with %d\n", val, valtocheck);

    if (val != valtocheck && (millis() - *prevtime < duration))
    {
        Serial.println("val != valtocheck && (millis() - *prevtime < duration)");
        return -1;
    }
    else if (val == valtocheck && (millis() - *prevtime >= duration))
    {
        Serial.printf("stage complete %d => %d", this->state, state_to_go);
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