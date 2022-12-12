#include <EEPROM.h>

void setup() {
  // put your setup code here, to run once:
  Serial.setRx(PA10);
  Serial.setTx(PA9);

  Serial.begin(9600);
  Serial.println("Test");

  Serial.println(EEPROM.read(0));
}

void loop() {
  // put your main code here, to run repeatedly:
  
}
