/*--------------------------------------------------------------------
Author		: Pedro Pereira
License		: BSD
Repository	: https://github.com/ppedro74/Arduino-SerialCommands

SerialCommands - Playing with Arguments

--------------------------------------------------------------------*/
#include <Arduino.h>
#include "SerialCommands.h"
#include <string.h>
#include <stdlib.h>

//Arduino UNO PWM pins
const int kRedLedPin = PA2;
const int kGreenLedPin = PA1;
const int kBlueLedPin = PA0;

char serial_command_buffer_[32];
SerialCommands serial_commands_(&Serial, serial_command_buffer_, sizeof(serial_command_buffer_), "\r", " ");

//This is the default handler, and gets called when no other command matches. 
void cmd_unrecognized(SerialCommands* sender, const char* cmd)
{
	sender->GetSerial()->print("Unrecognized command [");
	sender->GetSerial()->print(cmd);
	sender->GetSerial()->println("]");
}

//helper function pass led string (lower case) and pwm value
bool set_led(char* led, int pwm)
{
	pwm = 255 - pwm;
	int pin = - 1;

	//note: compare is case sensitive
	if (strcmp(led, "r") == 0)
	{
		pin = kRedLedPin;
	}
	else if (strcmp(led, "g") == 0)
	{
		pin = kGreenLedPin;
	}
	else if (strcmp(led, "b") == 0)
	{
		pin = kBlueLedPin;
	}

	if (pin<0)
	{
		//invalid pin
		return false;
	}

	if (pwm <= 0)
	{
		digitalWrite(pin, LOW);
	}
	else if (pwm >= 255)
	{
		digitalWrite(pin, HIGH);
	}
	else
	{
		analogWrite(pin, pwm);
	}

	return true;
}


//First parameter pwm value is required
//Optional parameters: led colors
//e.g. LED 128 red
//     LED 128 red blue
//     LED 0 red blue green
void cmd_rgb_led(SerialCommands* sender)
{
	//Note: Every call to Next moves the pointer to next parameter

	char* pwm_str = sender->Next();
	if (pwm_str == NULL)
	{
		sender->GetSerial()->println("ERROR NO_PWM");
		return;
	}
	int pwm = atoi(pwm_str);

	char* led_str;
	while ((led_str = sender->Next()) != NULL)
	{
		if (set_led(led_str, pwm))
		{
			sender->GetSerial()->print("LED_STATUS ");
			sender->GetSerial()->print(led_str);
			sender->GetSerial()->print(" ");
			sender->GetSerial()->println(pwm);
		}
		else
		{
			sender->GetSerial()->print("ERROR ");
			sender->GetSerial()->println(led_str);
		}
	}
}


SerialCommand cmd_rgb_led_("l", cmd_rgb_led);

void setup() 
{
	Serial.setRx(PA10);
	Serial.setTx(PA9);
	Serial.begin(57600);

	pinMode(kRedLedPin, OUTPUT);
	pinMode(kGreenLedPin, OUTPUT);
	pinMode(kBlueLedPin, OUTPUT);

	set_led("red", 0);
	set_led("green", 0);
	set_led("blue", 0);

	serial_commands_.SetDefaultHandler(cmd_unrecognized);
	serial_commands_.AddCommand(&cmd_rgb_led_);

	Serial.println("Ready!");
}

void loop() 
{
	serial_commands_.ReadSerial();
}