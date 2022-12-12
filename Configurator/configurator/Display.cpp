#include "Display.h"
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Display::Display()
    : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET)
{
}

bool Display::setupDisplay(void)
{
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
    {
        return false;
    }
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    return true;
}

void Display::displayString(const char *text)
{
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextColor(SSD1306_WHITE);
    display.print(text);
    display.display();
}


void Display::displayString(const __FlashStringHelper *ifsh)
{
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextColor(SSD1306_WHITE);
    display.print(ifsh);
    display.display();
}

void Display::displayHighlightedString(const char *text, int startPos, int len)
{
    display.clearDisplay();
    display.setCursor(0, 0);
    writeWhiteString(text, startPos);
    writeHighlightedString(text + startPos, len);
    writeWhiteString(text + startPos + len);
    display.display();
}

/// Can only display 1 line per 16 vertical pixels
/// No handling if the selectedItem is out of bounds, if the numTexts is incorrect,
/// or if the texts array contains nothing
void Display::displayTextLines(const char **texts, uint8_t numTexts, int8_t selectedItem)
{
    display.clearDisplay();
    display.setCursor(0, 0);
    writeTextLines(texts, numTexts, selectedItem);
    display.display();
}

/// Can only display 1 line per 16 vertical pixels - 1 is taken by the header
/// No handling if the selectedItem is out of bounds, if the numTexts is incorrect,
/// or if the texts array contains nothing
void Display::displayTextLinesWithheader(const char **texts, uint8_t numTexts, int8_t selectedItem, const char* header)
{
    display.clearDisplay();
    display.setCursor(0, 0);
    writeWhiteString(header);
    writeWhiteString("\r\n");
    writeTextLines(texts, numTexts, selectedItem);
    display.display();
}

// Doesn't actually call display(), clear the display, or set the curosr,
// just writes tot he display buffer
void Display::writeTextLines(const char **texts, uint8_t numTexts, int8_t selectedItem)
{
    int8_t startItem = selectedItem - 1; // Tries to put the selected item on the second line
    if (startItem < 0)
        startItem = 0;
        
    uint8_t endItem = startItem + (SCREEN_WIDTH/16); // To account for 4 lines total
    if (endItem > numTexts - 1)
        endItem = numTexts - 1;

    // Iterates from startItem through endItem (not up to)
    for (int i = startItem; i <= endItem; ++i)
    {
        if(i == selectedItem)
            writeHighlightedString(texts[i]);
        else
            writeWhiteString(texts[i]);
        display.write("\r\n");
    }
    
}

void Display::writeHighlightedString(const char *text)
{
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    display.write(text);
}

void Display::writeHighlightedString(const char *text, int len)
{
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    display.write(text, len);
}

void Display::writeWhiteString(const char *text)
{
    display.setTextColor(SSD1306_WHITE);
    display.write(text);
}

void Display::writeWhiteString(const char *text, int len)
{
    display.setTextColor(SSD1306_WHITE);
    display.write(text, len);
}

void Display::testscrolltext(void)
{
    display.clearDisplay();

    display.setTextSize(2); // Draw 2X-scale text
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 0);
    display.print(F("scroll"));
    display.display(); // Show initial text
    delay(100);

    // Scroll in various directions, pausing in-between:
    display.startscrollright(0x00, 0x0F);
    delay(2000);
    display.stopscroll();
    delay(1000);
    display.startscrollleft(0x00, 0x0F);
    delay(2000);
    display.stopscroll();
    delay(1000);
    display.startscrolldiagright(0x00, 0x07);
    delay(2000);
    display.startscrolldiagleft(0x00, 0x07);
    delay(2000);
    display.stopscroll();
    delay(1000);
}

void Display::displayULogo(void) {
  display.clearDisplay();

  display.drawBitmap(
    (display.width()  - U_logo_width ) / 2,
    (display.height() - U_logo_height) / 2,
    U_logo, U_logo_width, U_logo_height, 1);
  display.display();
}