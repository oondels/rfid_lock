#include "Actuator.h"
#include <Arduino.h>

Actuator::Actuator(int relayPin, int buttonPin)
    : relayPin(relayPin), buttonPin(buttonPin), relayState(false), openTimestamp(0), openDuration(0) {}

void Actuator::begin()
{
  pinMode(relayPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  digitalWrite(relayPin, LOW);
}

void Actuator::open(unsigned long duration)
{
  digitalWrite(relayPin, HIGH);
  relayState = true;
  openTimestamp = millis();
  openDuration = duration;
}

void Actuator::close()
{
  digitalWrite(relayPin, LOW);
  relayState = false;
  openDuration = 0;
}

void Actuator::loop()
{
  // Auto close after duration
  if (relayState && openDuration > 0 && (millis() - openTimestamp >= openDuration))
  {
    close();
  }
  // Manual open button
  if (isButtonPressed() && !relayState)
  {
    open();
  }
}

bool Actuator::isButtonPressed()
{
  return digitalRead(buttonPin) == LOW;
}