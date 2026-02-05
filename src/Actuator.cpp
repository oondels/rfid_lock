#include "Actuator.h"
#include <Arduino.h>

Actuator::Actuator(int relayPin, int buttonPin)
    : relayPin(relayPin), buttonPin(buttonPin), relayState(false), openTimestamp(0), openDuration(0) {}

void Actuator::begin()
{
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);
  pinMode(buttonPin, INPUT_PULLUP);
}

void Actuator::open(unsigned long duration)
{
  Serial.println("Opening actuator");
  digitalWrite(relayPin, LOW);
  relayState = true;
  openTimestamp = millis();
  openDuration = duration;
}

void Actuator::close()
{
  Serial.println("Closing actuator");
  digitalWrite(relayPin, HIGH);
  relayState = false;
  openDuration = 0;
}

void Actuator::loop()
{
  // Auto close after duration
  if (relayState && openDuration > 0 && (millis() - openTimestamp >= openDuration))
  {
    Serial.println("Auto-closing actuator");
    close();
  }
  // Manual open button
  if (isButtonPressed() && !relayState)
  {
    Serial.println("Manual opening actuator");
    open();
  }
}

bool Actuator::isButtonPressed()
{
  return digitalRead(buttonPin) == LOW;
}
