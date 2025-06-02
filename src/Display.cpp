#include "Display.h"

const char *displayTitle = "Dass Automacao \n Controle";

Display::Display(Adafruit_SSD1306 *display) : display(display) {}

void Display::showAccess(bool authorized, const String &name)
{
  display->clearDisplay();
  display->drawRect(0, 0, SCREEN_WIDTH, 30, SSD1306_WHITE);
  display->setTextSize(1);
  display->setTextColor(SSD1306_WHITE);
  display->setCursor(5, 10);
  display->println(F(displayTitle));
  display->setTextSize(1);
  display->setCursor(5, 45);
  display->println(name.length() > 0 ? name : "Colaborador");
  display->setCursor(5, 55);
  display->println(authorized ? "Acesso Liberado!" : "Acesso Negado!");
  display->drawBitmap(110, 45, authorized ? check_icon : cross_icon, 8, 8, SSD1306_WHITE);
  display->display();
}

void Display::showMessage(const String &message, const String &message2)
{
  display->clearDisplay();
  display->drawRect(0, 0, SCREEN_WIDTH, 30, SSD1306_WHITE);
  display->setTextSize(1);
  display->setTextColor(SSD1306_WHITE);
  display->setCursor(5, 10);
  display->println(F(displayTitle));
  display->setTextSize(1);
  display->setCursor(10, 45);
  display->println(message);
  if (message2.length() > 0)
  {
    display->setCursor(10, 55);
    display->println(message2);
  }
  display->display();
}

void Display::defaultMessage(bool wifi, bool ws)
{
  display->clearDisplay();
  display->drawRect(0, 0, SCREEN_WIDTH, 30, SSD1306_WHITE);
  display->setTextSize(1);
  display->setTextColor(SSD1306_WHITE);
  display->setCursor(5, 10);
  display->println(F(displayTitle));
  display->setTextSize(1);
  display->setCursor(10, 35);
  display->println("Aproxime o cracha");

  // display->setCursor(10, 45);
  // display->println("Do Leitor");

  char connectionStatusMessage[32];
  snprintf(connectionStatusMessage, sizeof(connectionStatusMessage),
           "Ws: %s - Wifi: %s", ws ? "on" : "off", wifi ? "on" : "off");
  
  display->setCursor(10, 55);
  display->setTextSize(0.5);
  display->println(connectionStatusMessage);

  display->display();
}

const unsigned char Display::check_icon[] PROGMEM = {
    B00000000,
    B00000001,
    B00000010,
    B00010100,
    B00101000,
    B01010000,
    B10000000,
    B00000000};

const unsigned char Display::cross_icon[] PROGMEM = {
    B10000001,
    B01000010,
    B00100100,
    B00011000,
    B00011000,
    B00100100,
    B01000010,
    B10000001};