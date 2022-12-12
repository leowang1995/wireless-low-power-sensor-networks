/*
 * ARPA-E Project wireless communication device code.
 * 
 * Microcontroller: Teensy3.6
 * Uses Arduino environment
 * 
 * Author: Bryan Hatasaka, Merek Goodrich, Sayali Tope
 * Some skeleton and setup code from PJRC Paul Stoffregen 
 * Libraries:
 * RadioHead supplied with Teensyduino - modified to work with the hardware
 * Snooze supplied with Teensyduino
 * Sparkfun TeensyView
 * 
 * Hardware:
 * Teensy 3.6
 * Adafruit RFM95W LoRa module breakout
 * Sparkfun TeensyView OLED
 * 
 */

#include <TeensyView.h>
#include <Snooze.h>
#include "Arpa_RF95.h"

#define THIS_NODE_ID 25

#define SEND_OVER_LORA true
#define NOTIF_DISP_TIME_SEC 0
#define NOTIF_DISP_TIME_MIN 5
#define NOTIF_DISP_TIME_HR 0

// RFM95W LoRa breakout pins
#define RFM95_RST 4
#define RFM95_EN 5
#define RFM95_CS 10
#define RFM95_INT digitalPinToInterrupt(3)
#define RFM95_POWER 23
#define RFM95_FREQ 915.0

// Oled Pins
#define OLED_RESET 25
#define OLED_DC    24
#define OLED_CS    9
#define OLED_SCK   13
#define OLED_MOSI  11

#define BASE_ID_0 0
#define BASE_ID_1 1

// ===== Function Declarations =====
void oled_init();
void oled_print(const char *text);
void oled_print_error(const char *text, int code);

void snooze_init();

bool SendLoraMessage(char* data);
bool SendLoraMessageWithDelayedRetry(char *data);
bool SendLoraGasNotif();

// ===== Global Variables =====
// OLED vars
TeensyView oled(OLED_RESET, OLED_DC, OLED_CS, OLED_SCK, OLED_MOSI);
char oled_buf[128];

// Snooze variables
const uint8_t sensor_wakeup_pin = 21;
const uint8_t button_wakeup_pin = 22;
SnoozeDigital digital;
SnoozeAlarm  alarm;
SnoozeBlock config(digital);
SnoozeBlock timerConfig(alarm, digital);

// Lora configuration and vars
// Singleton instance of the lora radio driver
RH_RF95 driver(RFM95_CS, RFM95_INT);
// Class to manage message delivery and receipt, using the driver declared above
Arpa_RF95 lora(&driver, RFM95_RST, RFM95_FREQ, RFM95_POWER, RFM95_EN, THIS_NODE_ID);

// Variables for interrupt

void setup()
{ 
  // Setup every pin as output at first
  for (int i=0; i<57; i++) {
    pinMode(i, OUTPUT);
  }
  
  // Init Snooze
  snooze_init();

  // Init OLED and Serial
  oled_init();
  Serial.begin(9600);

  oled_print("University of Utah");
  delay(2000); // Wait for Serial but don't require it
  oled.clear(PAGE);
  oled.clear(ALL);
  
  // Init LoRa
  Serial.print("Node ");
  Serial.print(THIS_NODE_ID);
  Serial.println(" starting");

  if(SEND_OVER_LORA)
  {
    if (!lora.InitModule())
    {
      Serial.println("LoRa couldn't be initialized");
      oled_print_error("LoRa fail init", 300);
      while(1) {}
    }
    else
    {
      Serial.println("LoRa initialized successfully");
    }
  }

  // Sleep LoRa module
  lora.SetSleepState(true);
}

// LoRa loop vars
char buf[ARPA_MAX_MSG_LENGTH];
uint8_t len = ARPA_MAX_MSG_LENGTH;
Arpa_msg_type msgType = ARPA_TYPE_ID_SYN;

// ===== Superloop =====
void loop()
{
  oled.clear(ALL);
  int wakeup_source = Snooze.deepSleep(config); // Sleep until woken by either the button or sensor
  int wakeupPinState = digitalRead(wakeup_source);
  unsigned int count_gas_detected = 1;
  bool show_message = true;
  unsigned long last_detection = 0;
  unsigned long  current_detection_time = millis();
  
  while(show_message)
  {
    switch(wakeup_source)
    {
      // 21 is gas, 22 is button
      // Write out gas detected and how many times it's been triggered
      case 21:
      case 22:
        // Debounce
        // Break out if this isn't the first detection and we have
        // another detection under 20 milliseconds
        if(count_gas_detected > 1 && current_detection_time - last_detection < 20)
          break;

        if(wakeupPinState == HIGH)
        {
          sprintf(oled_buf, "Gas detected\n%d", count_gas_detected);
          ++count_gas_detected;
          oled_print(oled_buf);
  
          
          // Send notification of gas
          if(SEND_OVER_LORA)
          {
            delay(2000); // Delay for 2 seconds to show gas message
            if(lora.SetSleepState(false)) // Wake up module
            {
              oled_print("Gas detected\nSending data to 0");
              lora.SetBaseId(BASE_ID_0);
              if(SendLoraGasNotif())
              {
                oled_print("Gas detected\nData sent!");
              }
              else
              {
                oled_print_error("Send failed", 302);
                oled_print("Gas detected");
              }
              
              oled_print("Sending data to 1");
              lora.SetBaseId(BASE_ID_1);
              if(SendLoraGasNotif())
              {
                oled_print("Gas detected\nData sent!");
              }
              else
              {
                oled_print_error("Send failed", 302);
                oled_print("Gas detected");
              }
            }
            else
              oled_print_error("LoRa fail init", 300);
            lora.SetSleepState(true); // Sleep module

            // check pin state and turn off the display if the 
            // pin has gone low in the time that it took to send the lora message
            if(digitalRead(wakeup_source) == LOW)
              show_message = false;

          }
        }
        // If the pin changed to a low state
        // turn off the display
        else
        {
          show_message = false;
        }

//        // Sleep for a bit to show message
//        wakeup_source = Snooze.deepSleep(timerConfig);
        break;

      // Break out of this loop and sleep without alarm
      // if woken up by the alarm
      default:
        show_message = false;
    }

    if(show_message)
    {
      // Sleep until we're woken by a pin change
      last_detection = current_detection_time;
      int wakeup_source = Snooze.deepSleep(config);
      wakeupPinState = digitalRead(wakeup_source);
      current_detection_time = millis();
    }
  }
}

bool SendLoraMessageWithDelayedRetry(char *data)
{
  while(true)
  {
    // Send the message and make sure it sent
    bool success = SendLoraMessage(data);
    
    if(success)
    {
      lora.ResetFailureToSendDelay();
      return true;
    }
    else
    {
      // Unsuccessful - delay according to protocol then resend
      if(!lora.FailureToSendDelay())
      {
        // Tried and failed to send too many times
        // give up
        lora.ResetFailureToSendDelay();
        return false;
      }
    }
  }
}

bool SendLoraMessage(char* data)
{
  Serial.println("Calling Synchronize()");
  if (lora.Synchronize())
  {
    Serial.println();
    Serial.println("===== Sync Success =====");
    Serial.println();

    Serial.println("===== Calling SendMessage() and sending a data type message =====");
    switch (lora.SendConnectedMessage(lora.GetBaseId(), ARPA_TYPE_ID_DATA, data))
    {
    case ARPA_TYPE_ID_ACK:
      // Got a good response from the base
      Serial.println("===== Data Success! =====");

      Serial.println("===== Calling lora.Close() =====");

      if (lora.Close())
      {
        Serial.println("===== Close Success =====");
        return true;
      }
      else
        Serial.println("===== Could not close connection =====");
      break;

    case ARPA_TYPE_ID_NACK:
      // Got a nack - we don't have a current connection with the base
      Serial.println("===== Data fail - not connected to base =====");
      oled_print_error("Got NACK", 301);
      return false;
      break;

    case ARPA_TYPE_ID_INVALID:
    default:
      Serial.println("===== Data message failed :( =====");
      return false;
      break;
    }
  }
  else
  {
    Serial.println("===== Could not synchronize =====");
    oled_print("Can't sync");
    delay(500);
    return false;
  }
  return true;
}

bool SendLoraGasNotif()
{
  sprintf(buf, "gas=1");
  return SendLoraMessage(buf);
}

void oled_init()
{
  oled.begin();    // Initialize the OLED
  oled.clear(ALL); // Clear the display's internal memory
  oled.clear(PAGE); // Clear the buffer.
}

void oled_print(const char *text)
{
  oled.clear(PAGE);  // Clear the page
  oled.setFontType(1);  // Set font to type 1
  oled.setCursor(0, 0); // move cursor
  oled.print(text);  // Write a string

  oled.display();  // Send the PAGE to the OLED memory
}

// Prints out the error text for 5 seconds, then
// the code for another 5 seconds
void oled_print_error(const char *text, int code)
{
  sprintf(oled_buf, "Problem!\n%s", text);
  oled_print(oled_buf);
  delay(5000);
  sprintf(oled_buf, "Problem!\nError code %d", code);
  oled_print(oled_buf);
  delay(5000);
}

void snooze_init()
{
  alarm.setRtcTimer(NOTIF_DISP_TIME_HR, NOTIF_DISP_TIME_MIN, NOTIF_DISP_TIME_SEC); // hr, min, sec
  
  digital.pinMode(sensor_wakeup_pin, INPUT, CHANGE);
  digital.pinMode(button_wakeup_pin, INPUT_PULLUP, LOW);
}
