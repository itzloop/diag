#pragma once

#include <Arduino.h>

#define DebugPrintln(a) Serial.println(a)
#define SUPPORTED_PID_01_TO_20 0x00188000
#define SUPPORTED_PID_21_TO_40 0x00000000
#define SUPPORTED_PID_41_TO_60 0x00000000
#define SUPPORTED_PID_61_TO_80 0x00000000
#define SUPPORTED_PID_81_TO_A0 0x00000000
#define SUPPORTED_PID_A1_TO_C0 0x00000000
#define SUPPORTED_PID_C1_TO_E0 0x00000000
#define SUPPORTED_SERVICE 0x01

enum ECU_STATE
{
    SLEEP,
    FIRST_INIT,
    SECOND_INIT,
    THIRD_INIT,
    MESSAGE_INIT,
    IDLE
};

typedef struct ecu_pids
{
    uint8_t mode;
    uint8_t pid;
    uint8_t len;
} ecu_pid_t;

class ECU
{
private:
    HardwareSerial *_serial;
    uint8_t tx_pin;
    uint8_t rx_pin;

    HardwareSerial *debuger;

    int _prevtime;
    bool _has_initialized;
    enum ECU_STATE state;
    int supportedList[] = {
        SUPPORTED_PID_01_TO_20,
        SUPPORTED_PID_21_TO_40,
        SUPPORTED_PID_41_TO_60,
        SUPPORTED_PID_61_TO_80,
        SUPPORTED_PID_81_TO_A0,
        SUPPORTED_PID_A1_TO_C0,
        SUPPORTED_PID_C1_TO_E0};

    int wakeup_state_helper(int *, int, int, enum ECU_STATE);
    bool supportService(uint8_t service);
    bool supportPid(uint8_t pid);

public:
    ECU();
    void init(HardwareSerial &serial, uint8_t tx, uint8_t rx);
    void writeInterval();
    bool wakeup();
    bool loop();
};

bool supportService(uint8_t service)
{
    return service == SUPPORTED_SERVICE;
}

bool supportPid(uint8_t pid)
{
    if (pid % 32 == 0)
        return true;

    if (pid > 0xE0)
        return false;

    uint8_t idx = pid / 32;
    int num = supportedList[idx];

    // std::cout << std::hex << num << std::endl;

    return ((num >> (32 - (pid % 32))) & 0x1) == 1;
}
