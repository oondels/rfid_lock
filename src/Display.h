#pragma once
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

class Display {
public:
    Display(Adafruit_SSD1306* display);
    void showAccess(bool authorized, const String& name = "", unsigned long delay = 0);
    void showMessage(const String& message, const String &message2, unsigned long delay = 0);
    void defaultMessage(bool wifi, bool ws);
private:
    Adafruit_SSD1306* display;
    unsigned long messageDelay = 1000;
    unsigned long lastMessageTime = 0;
    static const unsigned char check_icon[];
    static const unsigned char cross_icon[];
};