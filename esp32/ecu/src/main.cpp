#include <Arduino.h>
#include "ecu.h"

#define SERIAL2_RX_PIN 16
#define SERIAL2_TX_PIN 17

ECU ecu;

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("ecu simulaion started.");
  ecu.init(Serial2, SERIAL2_TX_PIN, SERIAL2_RX_PIN);
}

void loop()
{

  if (!ecu.initialized())
  {
    if (!ecu.wakeup())
    {
      // Serial.println("shit");
    }
    return;
  }

  ecu.loop();
}