#pragma once

class Actuator {
public:
    Actuator(int relayPin, int buttonPin);
    void begin();
    void open(unsigned long duration = 2000);
    void close();
    void loop();
    bool isButtonPressed();
private:
    int relayPin;
    int buttonPin;
    bool relayState;
    unsigned long openTimestamp;
    unsigned long openDuration;
};
