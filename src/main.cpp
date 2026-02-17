#include <ArduinoJson.h>
#include <ElegantOTA.h>
#include <EEPROM.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include "networkclient.h"
#include "Relay.h"

#define TEMP_READ_DELAY 60000
#define SENSOR_CHECK_DELAY 200000
#define DEFAULT_UPTIME 2700000
#define HOST_NAME "kelvin-switch"
#define MILISECONDS_FACTOR 60000

const char *MIME_TEXT = "text/plain";
const char *MIME_JSON = "application/json";
const char *SENSOR_PREFIX = "kelvin-sensor-";

float _targetTemperature = 0;
float _actualTemperature = 0;
byte _actualHumidity = 0;
unsigned long _lastSensorCheck = 0;
unsigned long _lastTemperatureCheck = 0;
unsigned int _heating_default_uptime = DEFAULT_UPTIME;
unsigned long _heating_max_uptime = _heating_default_uptime;
unsigned long _heating_uptime = 0;
unsigned long _heating_start_time = 0;
bool _heating_boost = false;
String _heating_start_datetime;
String _sensor_host;

AsyncWebServer server(80);
HTTPClient httpClient;
NetworkClient net;
Relay relay;
WiFiClient wifiClient;

void getSensorHost()
{
    if (_sensor_host != "")
    {
        return;
    }
    else if (_lastSensorCheck > 0 && millis() - _lastSensorCheck < SENSOR_CHECK_DELAY)
    {
        return;
    }

    _lastSensorCheck = millis();

    Serial.println(F("Looking for sensors via mDNS"));

    int serviceCount = MDNS.queryService("http", "tcp");

    for (int i = 0; i < serviceCount; i++)
    {
        if (String(MDNS.hostname(i)).startsWith(SENSOR_PREFIX))
        {
            Serial.print(F("Sensor found HOST: "));
            Serial.print(MDNS.hostname(i));
            Serial.print(F(" IP: "));
            Serial.println(MDNS.IP(i).toString());

            _sensor_host = MDNS.IP(i).toString();
            return;
        }
    }

    Serial.println(F("No temperature sensor found"));
}

void readTemperatureSensor()
{
    if (_sensor_host == "")
    {
        _actualHumidity = 0;
        _actualTemperature = 0;

        return;
    }

    httpClient.useHTTP10(true);
    httpClient.begin(wifiClient, _sensor_host, 80, "/api/status");

    int httpCode = httpClient.GET();

    if (httpCode == 200)
    {
        DynamicJsonDocument doc(128);

        deserializeJson(doc, httpClient.getStream());

        _actualHumidity = doc["humidity"].as<byte>();
        _actualTemperature = doc["actual_temperature"].as<float>();
    }
    else
    {
        Serial.print(F("Error reading sensor HTTP CODE: "));
        Serial.println(httpCode);
    }

    httpClient.end();
}

void startHeating(unsigned long uptime)
{
    relay.on();

    _heating_start_datetime = net.getDateTime();
    _heating_start_time = millis();
    _heating_uptime = 0;
    _lastTemperatureCheck = 0;
    _heating_max_uptime = uptime;

    Serial.println(F("Heating started"));
}

void startHeating()
{
    startHeating(_heating_default_uptime);
}

void stopHeating()
{
    relay.off();
    _heating_start_datetime = "";
    _heating_boost = false;
    _heating_uptime = 0;

    Serial.println(F("Heating stoped"));
}

void checkTemperature()
{
    if (_lastTemperatureCheck > 0 && millis() - _lastTemperatureCheck < TEMP_READ_DELAY)
    {
        return;
    }

    _lastTemperatureCheck = millis();

    readTemperatureSensor();

    Serial.print(F("Temperature check: "));
    Serial.println(_actualTemperature);

    if (relay.isOn())
    {
        if (_heating_uptime > _heating_max_uptime)
        {
            stopHeating();
        }
    }
    else
    {
        if (_actualTemperature > 0 && _targetTemperature >= _actualTemperature)
        {
            startHeating();
        }
    }
}

int getLeftUptime()
{
    if (!relay.isOn())
    {
        return 0;
    }

    return (_heating_max_uptime - _heating_uptime) / 60000;
}

double round2(double value)
{
    return (int)(value * 10 + 0.5) / 10.0;
}

void endpointSetTemperature(AsyncWebServerRequest *request)
{
    if (request->hasArg("target"))
    {
        _targetTemperature = request->arg("target").toFloat();
        _lastTemperatureCheck = 0;

        EEPROM.put(0, _targetTemperature);
        EEPROM.commit();

        request->send(200, MIME_TEXT, F("Target temperature set."));
    }
    else
    {
        request->send(400, MIME_TEXT, F("Target parameter not provided."));
    }
}

void endpointSetUptime(AsyncWebServerRequest *request)
{
    if (request->hasArg("value"))
    {
        _heating_default_uptime = request->arg("value").toInt() * MILISECONDS_FACTOR;

        EEPROM.put(sizeof(unsigned int), _heating_default_uptime);
        EEPROM.commit();

        request->send(200, MIME_TEXT, F("Default uptime set."));
    }
    else
    {
        request->send(400, MIME_TEXT, F("Value in minutes not provided."));
    }
}

void endpointStatus(AsyncWebServerRequest *request)
{
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    DynamicJsonDocument json(256);
    json["target_temperature"] = round2(_targetTemperature);
    json["actual_temperature"] = round2(_actualTemperature);
    json["humidity"] = _actualHumidity;
    json["heating"] = relay.isOn();
    json["boosting"] = _heating_boost;
    json["uptime_left_minutes"] = getLeftUptime();
    json["started_at"] = _heating_start_datetime;
    json["sensor_host"] = _sensor_host;
    json["default_uptime"] = _heating_default_uptime / MILISECONDS_FACTOR;

    serializeJson(json, *response);
    request->send(response);
}

void endpointBoost(AsyncWebServerRequest *request)
{
    if (request->hasArg("uptime"))
    {
        _heating_boost = true;

        startHeating(request->arg("uptime").toInt() * 60000);

        request->send(200, MIME_TEXT, F("Boosting started."));
    }
    else
    {
        request->send(400, MIME_TEXT, F("Uptime parameter not provided."));
    }
}

void endpointStart(AsyncWebServerRequest *request)
{
    startHeating();

    request->send(200, MIME_TEXT, F("Heating started."));
}

void endpointStop(AsyncWebServerRequest *request)
{
    stopHeating();

    request->send(200, MIME_TEXT, F("Heating stopped."));
}

void setup()
{
    Serial.begin(9600);
    EEPROM.begin(6);

    net.connect();
    net.registerMdnsHost(F("kelvin-thermostat"));

    if (!LittleFS.begin())
    {
        Serial.println(F("An error has occurred while mounting LittleFS"));
    }

    Serial.println(F("LittleFS mounted successfully"));

    server.on("/api/status", endpointStatus);
    server.on("/api/heating/on", endpointStart);
    server.on("/api/heating/off", endpointStop);
    server.on("/api/heating/boost", endpointBoost);
    server.on("/api/heating", endpointSetTemperature);
    server.on("/api/uptime", endpointSetUptime);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/index.html", "text/html", false); });

    server.onNotFound([](AsyncWebServerRequest *request)
                      { request->send(404, MIME_TEXT, "Not found"); });

    server.serveStatic("/", LittleFS, "/");

    ElegantOTA.begin(&server);
    server.begin();

    unsigned int uptime;
    float temperature;

    EEPROM.get(0, temperature);
    EEPROM.get(sizeof(unsigned int), uptime);

    if (isnan(temperature))
    {
        _targetTemperature = 0;

        Serial.println(F("Target temperature not set, using default"));
    }
    else
    {
        _targetTemperature = temperature;

        Serial.print(F("Target temperature set: "));
        Serial.println(_targetTemperature);
    }

    if (isnan(uptime))
    {
        _heating_default_uptime = DEFAULT_UPTIME;

        Serial.println(F("Heating uptime not set, using default"));
    }
    else
    {
        _heating_default_uptime = uptime;

        Serial.print(F("Heating uptime set: "));
        Serial.println(_heating_default_uptime);
    }

    Serial.println(F("Server is ready"));

    getSensorHost();
}

void loop()
{
    net.check();

    _heating_uptime = millis() - _heating_start_time;

    getSensorHost();
    checkTemperature();
}
