#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <IPAddress.h>
#include "secrets.h"
#include "constants.h"

const char *topicTX = "topic/test/rx";
const char *topicRX = "topic/test/tx";
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASS;

void callback(char *topic, uint8_t *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

WiFiClient wifiClient;
PubSubClient client(wifiClient);

unsigned long prevMillis = 0;
unsigned long delayDurationMS = 1000;

void setup_wifi(const char *ssid, const char *pass)
{
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
}

void setup_mqtt_client()
{

  while (!client.connected())
  {
    if (client.connect(MQTT_ID, MQTT_USER, MQTT_PASS))
    {
      Serial.println("connected to mqtt");
      // client.publish(topicTX, "hello world");
      bool ok = client.subscribe(topicRX, 1);
      if (!ok)
      {
        Serial.println("well fuck!");
      }
      Serial.printf("subscribed to topic: %s\n", topicRX);

      break;
    }
    Serial.println("failed to connect");
    Serial.println(client.state());
    delay(5000);
  }
}

void toggleLed(void *params)
{

  unsigned long prev = 0, duration = 1000;
  for (;;)
  {
    if (millis() - prev > duration)
    {
      digitalWrite(2, !digitalRead(2));
      prev = millis();
    }
  }
}

void messaging(void *params)
{
  unsigned long prev = 0, duration = 1000;
  for (;;)
  {
    client.loop();
    if (millis() - prev > duration)
    {

      if (!client.connected())
        setup_mqtt_client();
      else
        client.publish(topicTX, "Hello World");

      prev = millis();
    }
  }
}

TaskHandle_t t1, t2;
unsigned long prev = 0, duration = 1000;
bool connected = false;

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(1000);

  Serial.println("program starting...");

  // ecu.init(Serial2, SERIAL2_RX_PIN, SERIAL2_TX_PIN);
  // xTaskCreate(readECU, "read", 10000, NULL, tskIDLE_PRIORITY, &t1);
  // xTaskCreate(wakeupECU, "wakeupECU", 10000, NULL, 0, &t2);
  // Serial2.end();

  // Serial2.setTimeout(50);
  // uint8_t response[6] = {0};
  // Serial2.readBytes(response, 6);
  // if (response[3] == 0xC1)
  // {
  //   Serial.println("ecu has woken up");
  // }

  // pinMode(LED_BUILTIN, OUTPUT);

  // setup_wifi(ssid, password);
  // client.setServer(MQTT_URL, MQTT_PORT)
  //     .setCallback(callback)
  //     .setBufferSize(4096);
  // setup_mqtt_client();
}

void loop()
{

  if (!connected)
  {
    // send init message

    if (Serial2.availableForWrite())
    {
      uint8_t message[5] = {0xC1, 0x33, 0xF1, 0x81, 0x66};
      Serial2.write(message, 5);
      // for (uint8_t i = 0; i < len; i++)
      // {
      //   Serial2.write(message,);
      // }
    }

    // int len = 5;
  }

  delay(1000);

  // if (Serial2.available() > 0)
  // {
  //   Serial.print("read from ecu: ");
  //   Serial.println(Serial2.readString());
  // }
  // if (millis() - prev > duration)
  // {

  //   prev = millis();
  // }
  // Serial1.println("hello serial 1");
  // String s = Serial.readString();
  // if (s.equals(""))
  // {
  //   Serial.println("shit");
  // }
  // else
  // {
  //   Serial.println(s);
  // }
  // // Serial.print("read from ecu: ");
  // // Serial.println(Serial2.readString());
  // delay(1000);
  // client.loop();
  // if (millis() - prev > duration)
  // {

  //   if (!client.connected())
  //     setup_mqtt_client();
  //   else
  //     client.publish(topicTX, "Hello World");

  //   prev = millis();
  // }
}

// void handleError(String url, String resp, int respCode)
// {
//   if (respCode > 0)
//   {
//     Serial.printf("\nresponse for %s has status of %d\n body: %s\n===========================", url.c_str(), respCode, resp);
//   }
//   else
//   {
//     Serial.printf("Error %d", respCode);
//   }
// }

// void whatIsMyIP()
// {
//   bool ok = client.begin(domain.c_str());
//   if (!ok)
//   {
//     Serial.println("Something went wrong");
//     return;
//   }
//   int code = client.GET();

//   handleError(domain, client.getString(), code);
//   client.end();
// }

// void sendOBDData()
// {
//   String payload = "{\"request\": \"0x12 0x34 0x45\",\"response\": \"0x67 0x78 0x98\"}";

//   bool ok = client.begin("http://192.168.1.102:8080/api");
//   if (!ok)
//   {
//     Serial.println("Something went wrong 2");
//     return;
//   }
//   int code = client.POST(payload);
//   handleError("http://192.168.1.102:8080/api", client.getString(), code);
//   client.end();
// }