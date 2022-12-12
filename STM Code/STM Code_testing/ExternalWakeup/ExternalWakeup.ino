/*
  ExternalWakeup

  This sketch demonstrates the usage of External Interrupts (on pins) to wakeup
  a chip in sleep mode. Sleep modes allow a significant drop in the power usage
  of a board while it does nothing waiting for an event to happen.
  Battery powered application can take advantage of these modes to enhance
  battery life significantly.

  In this sketch, pressing a pushbutton attached to pin will wake up the board.

  This example code is in the public domain.
*/

#include "STM32LowPower.h"

// Blink sequence number
// Declare it volatile since it's incremented inside an interrupt
volatile int repetitions = 1;

// Pin used to trigger a wakeup
const int pin = PB0;
const int led = PA0;

void setup() {  
  pinMode(led, OUTPUT);
  pinMode(PA1, OUTPUT);
  // Set pin as INPUT_PULLUP to avoid spurious wakeup
  pinMode(pin, INPUT_PULLUP);

  digitalWrite(led, HIGH);
  digitalWrite(PA1, HIGH);

  // Configure low power
  LowPower.begin();
  // Attach a wakeup interrupt on pin, calling repetitionsIncrease when the device is woken up
  LowPower.attachInterruptWakeup(pin, repetitionsIncrease, CHANGE, DEEP_SLEEP_MODE );
}

void loop() {
  for (int i = 0; i < repetitions; i++) {
    digitalWrite(led, LOW);
    delay(100);
    digitalWrite(led, HIGH);
    delay(100);
  }
  // Triggers an infinite sleep (the device will be woken up only by the registered wakeup sources)
  // The power consumption of the chip will drop consistently
  __HAL_RCC_GPIOA_CLK_DISABLE();
  LowPower.deepSleep();
  __HAL_RCC_GPIOA_CLK_ENABLE();
}

void repetitionsIncrease() {
  // This function will be called once on device wakeup
  // You can do some little operations here (like changing variables which will be used in the loop)
  // Remember to avoid calling delay() and long running functions since this functions executes in interrupt context
  repetitions ++;
}
