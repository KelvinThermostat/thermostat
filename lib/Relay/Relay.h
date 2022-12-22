#include <Arduino.h>

class Relay
{
public:
    void off();
    void on();
    bool isOn();
    Relay();
};
