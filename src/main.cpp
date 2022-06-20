#include <Arduino.h>
#include <WiFi.h>
#include "HtmlResponse.h"
//////////////////// String management functions /////////////////////////////////////

int GetStringUntilSeparator(const char* inString, int beginIndex, char separator, char* outString)
{
  char c;
  inString += beginIndex;
  int count = 0;
  while (count != 30)
  {
    c = *inString;
    if (c == separator)
    {
      break;
    }

    *outString += c;
    outString++;
    count++;
    inString++;
  }

  return count;
}

int FindIndexOfString(const char* string, int stringSize, const char* stringToFind, int stringFindSize, int beginIndex)
{
  int index = -1;

  int count = beginIndex;
  int foundCount = 0;
  string += beginIndex;

  while (count < stringSize)
  {
    if(stringToFind[foundCount] == *string)
    {
      foundCount++;
      if(foundCount == 0)
      {
        index = count;
      }
    }
    else
    {
      foundCount = 0;
      index = -1;
    }

    if(foundCount == stringFindSize)
    {
      break;
    }

    count++;
    string++;
  }

  return index;
}


////////////////////////////////// Http AP connection functions //////////////////////////////////
const char *ssid = "ESP32-Access-Point";
const char *password = "123456789";

char* setupPassword = {0};
char* setupSsid = {0};

WiFiServer server(80);

// Set your Static IP address
IPAddress local_IP(192, 168, 1, 184);
// Set your Gateway IP address
IPAddress gateway(192, 168, 1, 1);

IPAddress subnet(255, 255, 0, 0);
IPAddress primaryDNS(8, 8, 8, 8);   // optional
IPAddress secondaryDNS(8, 8, 4, 4); // optional

uint32_t m_LastEventTime = 0;


enum WIFI_SETUP_ESP
{
  ESP_AP_NOT_CONNECTED = 0,
  ESP_AP_CONNECTED,
  ESP_HAS_SETUP,
  ESP_IS_CONNECTED
};

WiFiClient client;

WIFI_SETUP_ESP m_SetupState = ESP_AP_NOT_CONNECTED;
void SetSoftApWifi()
{
  WiFi.mode(WIFI_MODE_AP);
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  server.begin();
}

void setup()
{
  Serial.begin(115200);

  SetSoftApWifi();
}

void ReadClientMessage(char* outString)
{
  unsigned int count = client.available();
  char* stringBegin = outString;
  while (count > 0)
  {
    char c = client.read();
    *outString = c;
    outString++;
    count--;
  }

  Serial.println("Recebido: ");
  Serial.println(stringBegin);
}

char HandleSetupMessage()
{
  char out = 0;
  if (!client)
  {
    client = server.available();
  }
  else
  {
    if (client.connected() && server.available() > 0)
    {
      char httpMessage[200] = {0};
      
      ReadClientMessage(httpMessage);
      if (FindIndexOfString(httpMessage, 200, "POST /setup", sizeof("POST /setup"), 0) >= 0)
      {
        Serial.println("\nPost message\n");
        int indexOfMessage = FindIndexOfString(httpMessage, 200, "\r\n\r\n", sizeof("\r\n\r\n"), 0);

        if (indexOfMessage >= 0)
        {
          char newSsid[30] = {0};
          char ssidSize = 0;
          char newPassword[30] = {0};
          char passwordSize = 0;

          int ssidIndex = FindIndexOfString(httpMessage, 200, "ssid=", sizeof("ssid="), indexOfMessage);

          if (ssidIndex >= 0)
          {
            ssidSize = GetStringUntilSeparator(httpMessage, ssidIndex, '&', newSsid);
            unsigned int newPasswordIndex = ssidIndex + 6 + ssidSize;
            passwordSize = GetStringUntilSeparator(httpMessage, newPasswordIndex, '&', newPassword);
          }

          if (ssidSize != 0 && passwordSize != 0)
          {
            memcpy(setupPassword, newPassword, passwordSize);
            memcpy(setupSsid, newSsid, ssidSize);
            HtmlPageSucessSend(client);
            out = 1;
          }
        }
      }
      else if (FindIndexOfString(httpMessage, 200, "GET /", sizeof("GET /"), 0) >= 0)
      {
        HtmlMainPageSend(client);
      }

      server.stop();
      
    }
  }
  return out;
}

void SetupLoop()
{
  int disconnect = 0;
  switch (m_SetupState)
  {
   case ESP_AP_NOT_CONNECTED:
    if (millis() - m_LastEventTime > 5000)
    {
      SetSoftApWifi();
      m_LastEventTime = millis();
    }
    if (WiFi.status() == WL_CONNECTED)
    {
      m_LastEventTime = millis();
      m_SetupState = ESP_AP_CONNECTED;
    }
    break;

  case ESP_AP_CONNECTED:
    if(HandleSetupMessage())
    {
      m_SetupState = ESP_HAS_SETUP;
    }
    break;

  case ESP_HAS_SETUP:
    if(WiFi.status() == WL_CONNECTED)
    {
      m_SetupState = ESP_IS_CONNECTED;
      Serial.println("Conectado");
    }
    else if(WiFi.status() == WL_CONNECT_FAILED)
    {
      Serial.println("Conecção recusada...");
      SetSoftApWifi();
      m_SetupState = ESP_AP_NOT_CONNECTED;
    }

    break;

  case ESP_IS_CONNECTED:
    break;  

  default:
    break;
  }
}

void loop()
{
  SetupLoop();
}
