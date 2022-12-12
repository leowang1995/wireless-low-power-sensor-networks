/*
 * Project LTE
 * Description:
 * Author:
 * Date:
 */
// Disable optimization for debuggin
// #pragma GCC optimize ("O0")
#include "MQTT.h"

#define BASE_UART_BAUD 57600
#define TOPIC "arpa/msg/%d"
#define MQTT_DEVICE_NAME "arpa_boron_1"
#define MQTT_USER "sensor-node"
#define MQTT_PASS "SensorNode$"
#define MQTT_PORT 4000
#define BASE_ID ";1"       //set up the base ID
char *MQTT_DOMAIN = "104.131.65.189";
// char *MQTT_DOMAIN = "sensor-node.hatasaka.com";


void callback(char *topic, uint8_t *payload, unsigned int length);
void mqtt_connect();
void mqtt_publish(char *topic, char *msg);
void connect_celluar();

SerialLogHandler logHandler;
int led = D7; // The on-board LED
SystemSleepConfiguration sleepConfig;
MQTT client(MQTT_DOMAIN, MQTT_PORT, callback);

void setup()
{
  // Shut down peripherals we don't need
  BLE.off();

  // Setup pins
  pinMode(led, OUTPUT);
  Serial1.begin(BASE_UART_BAUD);

  // Configure sleep - keep usart on
  // so no data from the base is missed
  //
  // also keep the network module active
  // so that the SIM isn't banned fro agressive reconnections while testing
  sleepConfig.mode(SystemSleepMode::ULTRA_LOW_POWER)
      .usart(Serial1)
      .network(NETWORK_INTERFACE_CELLULAR, SystemSleepNetworkFlag::INACTIVE_STANDBY);

  Log.info("Starting");
  // connect to the server
  Log.info("Connecting to mqtt server");
  mqtt_connect();

  // publish/subscribe
  if (client.isConnected())
  {
    Log.info("Connected to mqtt server");
    client.publish("apra/init", "apra_boron_1");
  }
}

char msg[512]; // Oversize the buffer
char topic[16];
uint8_t nodeId = 0;

int idx = 0;
char uart_char = '\0';
bool start = true;
bool publish = false;

void loop()
{
  // Read data from base (if there is any)
  while (Serial1.available() > 0)
  {
    uart_char = Serial1.read();

    Log.info("%c", uart_char);
    if (start)
    {
      // very first byte is the nodeId
      // don't put into message buffer
      nodeId = uart_char;
      start = false;
    }
    else if (uart_char == ';')
    {
      // Terminator to message - time to publish
      start = true;
      publish = true;
      Log.info("Found serial terminator");
    }
    else if (idx >= 512 - 2)
    {
      // If no terminator has been found yet,
      // then publish what we have but don't look
      // for a new nodeId
      publish = true;
      msg[idx++] = uart_char;
    }
    else
    {
      msg[idx++] = uart_char;
    }

    if (publish)
    {
      // Null terminate then send the message
      // Reset and get ready to receive again
      msg[idx] = '\0';
      strcat(msg, BASE_ID);

      sprintf(topic, TOPIC, nodeId);
      Particle.publish(String::format(TOPIC, nodeId), String(msg));
      mqtt_publish(topic, msg);

      memset(msg, '\0', 512);
      memset(topic, '\0', 16);
      idx = 0;

      publish = false;
    }
  }

  // mqtt connection items
  if (client.isConnected())
  {
    client.loop();
  }

  // Sleep if there is no serial available
  if (Serial1.available() <= 0)
  {
    // Log.info("Going to sleep");
    // SystemSleepResult result = System.sleep(sleepConfig);
    // Log.info("Woke. Reason: %d", (int)result.wakeupReason());
  }
}

// recieve message
void callback(char *topic, uint8_t *payload, unsigned int length)
{
  digitalWrite(led, HIGH);
  delay(250);
  digitalWrite(led, LOW);
  delay(250);
  digitalWrite(led, HIGH);
  delay(250);
  digitalWrite(led, LOW);
  delay(250);
}

void mqtt_publish(char *topic, char *msg)
{
  Log.info("mqtt_publish called.");
  connect_celluar();
  if (!client.isConnected())
  {
    mqtt_connect();
  }
  Log.info("Publishing topic: %s\tmsg:%s", topic, msg);
  client.publish(topic, msg);
}

void mqtt_connect()
{
  for (auto tries = 0; tries < 10; ++tries)
  {
    if (client.connect(MQTT_DEVICE_NAME, MQTT_USER, MQTT_PASS))
    {
      Log.info("MQTT Connected");
      Particle.publish("MQTT connected");
      return;
    }
    else
    {
      Log.info("MQTT Couldn't connect");
      RGB.control(true);
      RGB.color(255, 0, 0);
      RGB.brightness(128);
      delay(500);
      RGB.control(false);
      delay(500);
    }
  }
  Particle.publish("MQTT could not connect after 10 tries.");
}

void connect_celluar()
{
  Log.info("Making sure celluar is connected");
  while (!Cellular.ready())
  {
    Cellular.connect();
    while (Cellular.connecting())
    {
      delay(1);
    }
  }
}
