#include "Relay.h"

const byte _relay_state_on[] = {0xA0, 0x01, 0x01, 0xa2};
const byte _relay_state_off[] = {0xA0, 0x01, 0x00, 0xa1};
const ulong _relay_max_uptime = 18000000;

ulong _relay_uptime = 0;
ulong _relay_start_time = 0;
bool _currentState;

void Relay::on()
{
    Serial.write(_relay_state_on, sizeof(_relay_state_on));
    _currentState = true;
    _relay_start_time = millis();
}

void Relay::off()
{
    Serial.write(_relay_state_off, sizeof(_relay_state_off));
    _currentState = false;
    _relay_start_time = 0;
    _relay_uptime = 0;
}

void Relay::check()
{
    if (_currentState)
    {
        _relay_uptime = millis() - _relay_start_time;

        if (_relay_uptime > _relay_max_uptime)
        {
            off();
        }
    }
}

bool Relay::isOn()
{
    return _currentState;
}

ulong Relay::getUpTime()
{
    return _relay_uptime;
}
