#include <Arduino.h>
#include "ecu.h"

#define RXPIN 0
#define TXPIN 1

ECU ecu;

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(10400);
  Serial.println("program starting");
  Serial.end();

  ecu.init(Serial, RXPIN, TXPIN);
  ecu.wakeup();
}

void loop()
{
  ecu.loop();
}