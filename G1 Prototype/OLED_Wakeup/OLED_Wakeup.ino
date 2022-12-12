#include <TeensyView.h>  // Include the SFE_TeensyView library
#include <Snooze.h>

// Oled Pins
#define OLED_RESET 25
#define OLED_DC    24
#define OLED_CS    9
#define OLED_SCK   13
#define OLED_MOSI  11

// ===== Funcation Declarations =====
void oled_init();
void oled_print(const char *text);
void snooze_init();

// ===== Global Variables =====
TeensyView oled(OLED_RESET, OLED_DC, OLED_CS, OLED_SCK, OLED_MOSI);
char oled_buf[128];

const uint8_t wakeup_pin = 21;
SnoozeDigital digital;
SnoozeBlock config(digital);

void setup()
{
  oled_init();
  snooze_init();

  oled_print("University of Utah");
  delay(1000);
  oled.clear(ALL);
}

void loop()
{
  Snooze.deepSleep( config ); // sleep

  // TODO: Detect an interrupt on the gas pin
  // just in case it is detected again while the devic is awake
  oled_init();
  oled_print("Woke up!");
  delay(500);
  for(int i = 3; i > 0; --i)
  {
    sprintf(oled_buf, "Going back to sleep in: %d", i);
    oled_print(oled_buf);
    delay(1000);
  }
  oled.clear(ALL);
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

void snooze_init()
{
  digital.pinMode(wakeup_pin, INPUT, RISING);
}
