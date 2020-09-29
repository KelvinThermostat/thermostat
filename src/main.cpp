#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "Relay.h"
#include "networkclient.h"

ESP8266WebServer server(80);
Relay relay;
NetworkClient net;

void showStatus()
{
  String content = "{\"on\":";

  if (relay.isOn())
  {
    content += "true,";
  }
  else
  {
    content += "false,";
  }

  content += "\"uptime\":";
  content += relay.getUpTime();
  content += "}";

  server.send(200, "application/json", content);
}

void start()
{
  relay.on();

  server.send(201);
}

void stop()
{
  relay.off();

  server.send(201);
}

void setup()
{
  Serial.begin(9600);

  String host = "kelvin-switch-";
  host += ESP.getChipId();

  net = NetworkClient(&host);

  net.connect();

  server.on("/status", showStatus);
  server.on("/on", start);
  server.on("/off", stop);
  server.begin();

  Serial.println("Server is ready");
}

void loop()
{
  net.check();
  relay.check();
  server.handleClient();
}
