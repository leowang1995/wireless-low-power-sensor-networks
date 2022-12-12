/// Header-only implementation of a basic button debounce library.
/// The 'read()' function for the button must be called consistently within a certain period.
/// Ideally it should be a consistent a period of 1-8 milliseconds and will be triggered by a timer.
/// 1ms poll period means a button press registers about 5ms after press.
/// 5ms poll period means a button press registers about 25ms after the press.
/// In order to accurately catch button presses, the pressed()/released() function must 
/// be checked after every poll as they only returns true for 1 poll cycle after the 
/// button has been pressed.
///
/// Designed for buttons that pull down when pressed.
/// Enables the internal pull-up resistor for any button pins.

#pragma once
#include "Arduino.h"

class Button
{
	public:
		Button(uint8_t pin)
		:  pin(pin), history(0xFF) {}

		void init() { pinMode(pin, INPUT_PULLUP); }
		/// Polling function
		void read() { history = (history << 1) | digitalRead(pin); }

		/// The history of all of the button polls is held in an 8-bit variable.
		/// The latest state is shifted into it from the right.
		inline uint8_t getHistory() { return history; }

		/// Only returns true for 1 poll cycle after the button has been pressed
		inline bool pressed() { return PRESSED_MASK == history; }

		/// Only returns true for 1 poll cycle after the button has been pressed
		inline bool released() { return RELEASED_MASK == history; }

		/// Allows for 3 reads of the button being up before it reads as up
		/// For some wiggle room of spurious reads or whatnot
		inline bool isDown() { return __builtin_popcount(history) <= 3; } // !!! AVRGCC specific

	private:
		uint8_t  pin;
		// Initialized as 0xFFFF as this means not pressed
		uint8_t history;
		static const uint8_t PRESSED_MASK = 0xE0;  // 0b_1110_0000
		static const uint8_t RELEASED_MASK = 0x1F; // 0b_0001_1111

};
