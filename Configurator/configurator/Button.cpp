// #include "Button.h"
// #include <Arduino.h>

// Button::Button(uint8_t pin)
// :  pin(pin), state(false), history(0)
// {
// }

// void Button::init()
// {
// 	pinMode(pin, INPUT_PULLUP);
// }

// // 
// // public methods
// // 

// void Button::read()
// {
// 	history = (history << 1) | digitalRead(pin);
// }

// // has the button gone from off -> on
// inline bool Button::pressed()
// {
// 	return PRESSED_MASK & history;
// }

// // has the button gone from on -> off
// inline bool Button::released()
// {
// 	return RELEASED_MASK & history;
// }
