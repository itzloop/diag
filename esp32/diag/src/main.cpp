#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <IPAddress.h>
#include "secrets.h"
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

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(1000);
  // pinMode(LED_BUILTIN, OUTPUT);

  setup_wifi(ssid, password);
  client.setServer("itsloop.dev", 8883)
      .setCallback(callback)
      .setBufferSize(4096);
  setup_mqtt_client();
}

void loop()
{
  client.loop();
  if (millis() - prevMillis > delayDurationMS)
  {
    if (!client.connected())
      setup_mqtt_client();
    else
      client.publish(topicTX, "Hello World");

    prevMillis = millis();
  }
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