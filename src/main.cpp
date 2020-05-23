#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266WebServerSecure.h>
#include "Relay.h"

const char *ssid = "BaconNet";
const char *password = "5AlandaleClose//";
const char *dname = "kelvin-controller";

ESP8266WebServer server(80);
BearSSL::ESP8266WebServerSecure serverHTTPS(443);
Relay relay;

static const char serverCert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIICTzCCAbgCCQCWlxFdwUAKjDANBgkqhkiG9w0BAQsFADBsMQswCQYDVQQGEwJJ
RTETMBEGA1UECAwKTD1MaW1lcmljazEUMBIGA1UECgwLS2VsdmluIFtJRV0xEDAO
BgNVBAsMB1NtYXJ0ZXIxIDAeBgNVBAMMF2tlbHZpbi1jb250cm9sbGVyLmxvY2Fs
MB4XDTIwMDUxNDE2MzU0MVoXDTI1MDUxMzE2MzU0MVowbDELMAkGA1UEBhMCSUUx
EzARBgNVBAgMCkw9TGltZXJpY2sxFDASBgNVBAoMC0tlbHZpbiBbSUVdMRAwDgYD
VQQLDAdTbWFydGVyMSAwHgYDVQQDDBdrZWx2aW4tY29udHJvbGxlci5sb2NhbDCB
nzANBgkqhkiG9w0BAQEFAAOBjQAwgYkCgYEAoEs/A2kv/nzPtBYbmP/7gXsV/F7n
JqrZrhd+FtVG0r7dydbw6qmAtkHU2qVbZSqBB37+jpYudyqXN56OY0r7odDdIEbi
LvCZJERsHFS1cuCHyh5zAa7cz+CiN7EgZVi6vtTUf28s/MBR3hIAn3d8wBmHjemn
xOJ5oCGCDd7d5r0CAwEAATANBgkqhkiG9w0BAQsFAAOBgQBfe3x8rntd/BE9Fdmo
RzbRZ6h/cx2AQk5gSmDgFY2NKGjdUPu3InW6bBO7ENPV+76ooE2R5oVuGv+SU1fi
fZrgCdzuTIRFZD5XnJ48QTnJreXJJorLkCb2IJyBeXp4tS0auFv6iqprIPMyNU2c
LTCgQZ57kmf3QjoUw6YZdcuzrw==
-----END CERTIFICATE-----
)EOF";

static const char serverKey[] PROGMEM = R"EOF(
-----BEGIN PRIVATE KEY-----
MIICdQIBADANBgkqhkiG9w0BAQEFAASCAl8wggJbAgEAAoGBAKBLPwNpL/58z7QW
G5j/+4F7Ffxe5yaq2a4XfhbVRtK+3cnW8OqpgLZB1NqlW2UqgQd+/o6WLncqlzee
jmNK+6HQ3SBG4i7wmSREbBxUtXLgh8oecwGu3M/gojexIGVYur7U1H9vLPzAUd4S
AJ93fMAZh43pp8TieaAhgg3e3ea9AgMBAAECgYBmudJYTUEExtgVgVWOZ/62rDsa
0XcxwKxgdY3I9EW/KbLZd3ZN7PMf2nCe7vnzi5nFRA2/M1/z3seqTWLTH2rC9YLe
k22hL5Antqg7pMSTltnZuEwfu9sZ7ijgsmZcoBJMfRtAM3+cbvGKcWyc1OWJVS06
/AzrFvPBLj+V9xmpwQJBAMrkv1SVPpGsZd42CfCAtfY4W8UfSLCump0b2/9qQN7u
LoRPOhQvlKj6gJyLSfaZ35/kYKofjrlM1T/RK92zedsCQQDKQAimKFbiq9iXdgiP
T2iVxdJMOSWu7NTzicdKru6BgpBqaiz7au0fWb727UyFpoU5ny1i+4npAue8h5rF
e8FHAkA3LXR3LUth1I+zmeCkHmzd9D/I4RQksKdtuKjg0mWn+wB9jQpPeQ0l01Js
wFiyDvDJDBPZ0FFBRYrtGxah6XBnAkB10OF48WzTtGmCqbaIzShOfyNnVa72/G18
xRj8D3VOqmE4LEux1fL13VDaBRgbjwpyq6BD1eXbf97Au3nUaqgHAkA0inJRoxzJ
mSQzV9P5Ce+B6nspMNJSMiGCpXeqcp7QDnonnvnIDklOjl4sRbz62Y/H+pkFS6Jh
Ze5rl6rRabt+
-----END PRIVATE KEY-----
)EOF";

bool connectToWifi()
{
  byte timeout = 50;

  Serial.println("\n\n");
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  for (int i = 0; i < timeout; i++)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("\nConnected to WiFi");
      Serial.print("Server can be accessed at https://");
      Serial.print(WiFi.localIP());

      if (MDNS.begin(dname))
      {
        Serial.print(" or at https://");
        Serial.print(dname);
        Serial.println(".local");
      }

      return true;
    }

    delay(1000);
    Serial.print(".");
  }

  Serial.println("\nFailed to connect to WiFi");
  Serial.println("Check network status and access data");
  Serial.println("Push RST to try again");

  return false;
}

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

  if (!connectToWifi())
  {
    delay(2000);
    ESP.restart();
  }

  configTime(1 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  serverHTTPS.getServer().setRSACert(new BearSSL::X509List(serverCert), new BearSSL::PrivateKey(serverKey));
  server.on("/status", showStatus);
  server.on("/on", start);
  server.on("/off", stop);
  server.begin();

  Serial.println("Server is ready");
}

void loop()
{
  relay.check();
  server.handleClient();
  MDNS.update();
}
