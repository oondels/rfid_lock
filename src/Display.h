#pragma once
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

class Display {
public:
    Display(Adafruit_SSD1306* display);
    void showStatus(const String& line1, const String& line2 = "");
    void showAccess(bool authorized, const String& name = "");
    void showMessage(const String& message, const String &message2);
private:
    Adafruit_SSD1306* display;
    static const unsigned char check_icon[];
    static const unsigned char cross_icon[];
};