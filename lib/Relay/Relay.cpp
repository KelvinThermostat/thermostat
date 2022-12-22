#include "Relay.h"

#ifdef RELAY_SERIAL
const byte _relay_state_on[] = {0xA0, 0x01, 0x01, 0xa2};
const byte _relay_state_off[] = {0xA0, 0x01, 0x00, 0xa1};
#endif

#define RELAY_PIN 5

bool _isOn = false;

Relay::Relay()
{
    pinMode(RELAY_PIN, OUTPUT);
}

void Relay::on()
{
#ifdef RELAY_SERIAL
    Serial.write(_relay_state_on, sizeof(_relay_state_on));
#else
    digitalWrite(RELAY_PIN, HIGH);
#endif

    _isOn = true;
}

void Relay::off()
{
#ifdef RELAY_SERIAL
    Serial.write(_relay_state_off, sizeof(_relay_state_off));
#else
    digitalWrite(RELAY_PIN, LOW);
#endif

    _isOn = false;
}

bool Relay::isOn()
{
    return _isOn;
}
