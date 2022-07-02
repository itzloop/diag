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

#define INTERNAL_BUFFER_SIZE 16

struct ecu_data
{
    // 0 to 16,383
    uint8_t engineSpeed[2];

    // 0 to 255
    uint8_t vehicleSpeed[1];

    // 0 to 100
    uint8_t throttle[1];
};

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

    uint8_t buffer[INTERNAL_BUFFER_SIZE];

    int _prevtime;
    bool _has_initialized;
    enum ECU_STATE state;

    struct ecu_data data;

    int wakeup_state_helper(int *, int, int, enum ECU_STATE);
    bool supportService(uint8_t service);
    bool supportPid(uint8_t pid);
    void send_reponse(uint8_t *data, int datalen);

public:
    ECU();
    void init(HardwareSerial &serial, uint8_t tx, uint8_t rx);
    bool wakeup();
    void loop();
    bool initialized();
};