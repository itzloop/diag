#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <IPAddress.h>
#include "secrets.h"
#include "constants.h"
#include "obd.h"

const char *topicTX = DIAG_TOPIC;
const char *topicRX = "topic/test/rx";
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

OBD2 diag;
TaskHandle_t t1, t2;
bool connected = false;

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
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println("program starting...");

  xTaskCreatePinnedToCore(messaging, "messaging", 10000, NULL, 1, &t1, 1);

  setup_wifi(ssid, password);
  client.setServer(MQTT_URL, MQTT_PORT)
      .setCallback(callback)
      .setBufferSize(4096);
  setup_mqtt_client();
  connected = diag.init(Serial2, SERIAL2_RX_PIN, SERIAL2_TX_PIN);
}

char format[] = "{\"engine\":%s,\"vehicle\":%s,\"throttle\":%s}";
char buf[4096];

void loop()
{
  if (!connected)
  {
    connected = diag.init(Serial2, SERIAL2_TX_PIN, SERIAL2_RX_PIN);
    delay(5000);
    return;
  }

  // get engine speed
  diag.getPid(0x0C, 0x01);
  String a = diag.humanReadable(0x0C, 0x01);

  // get vehicle speed
  diag.getPid(0x0D, 0x01);
  String b = diag.humanReadable(0x0D, 0x01);

  // get throttle
  diag.getPid(0x11, 0x01);
  String c = diag.humanReadable(0x11, 0x01);

  // marshal json
  sprintf(buf, format, a, b, c);
  Serial.println(buf);

  // send data to server
  if (!client.connected())
    setup_mqtt_client();
  else
    client.publish(topicTX, buf);

  delay(3000);

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