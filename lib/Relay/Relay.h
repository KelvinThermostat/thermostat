#include <Arduino.h>

class Relay
{
public:
    void off();
    void on();
    void check();
    ulong getUpTime();
    bool isOn();
};
