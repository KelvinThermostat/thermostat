#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "Relay.h"
#include "networkclient.h"

ESP8266WebServer server(80);
Relay relay;
NetworkClient net;
String host = "kelvin-switch-";

void endpointStatus()
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

void endpointStart()
{
  relay.on();

  server.send(201);
}

void endpointStop()
{
  relay.off();

  server.send(201);
}

void setup()
{
  Serial.begin(9600);

  host += ESP.getChipId();

  net = NetworkClient(&host);

  net.connect();

  server.on("/api/status", endpointStatus);
  server.on("/api/on", endpointStart);
  server.on("/api/off", endpointStop);
  server.begin();

  Serial.println("Server is ready");
}

void loop()
{
  net.check();
  relay.check();
  server.handleClient();
}
