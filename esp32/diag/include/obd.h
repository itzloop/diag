#pragma once
#include <Arduino.h>

#define BUFFER_SIZE 16
// #define ECU_WAKEUP_INIT

class OBD2
{
private:
    HardwareSerial *_serial;
    uint8_t rx;
    uint8_t tx;

    // void write(uint8_t b);
    // // writes a byte and removes the echo.

    // void write(void *b, uint8_t len);
    // // writes an array and removes the echo.

    uint8_t buffer[BUFFER_SIZE];

public:
    void getPid(uint8_t pid, uint8_t mode);
    bool init(HardwareSerial &serial, uint8_t tx, uint8_t rx);
    String humanReadable(uint8_t pid, uint8_t mode);
};