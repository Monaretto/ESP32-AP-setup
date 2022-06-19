#include <Arduino.h>
#include <WiFi.h>

// Replace with your network credentials
const char *ssid = "ESP32-Access-Point";
const char *password = "123456789";

WiFiServer server(80);

// Set your Static IP address
IPAddress local_IP(192, 168, 1, 184);
// Set your Gateway IP address
IPAddress gateway(192, 168, 1, 1);

IPAddress subnet(255, 255, 0, 0);
IPAddress primaryDNS(8, 8, 8, 8);   // optional
IPAddress secondaryDNS(8, 8, 4, 4); // optional

struct WifiConnectInfo
{
  String s_Ssid;
  String s_Password;
};

WifiConnectInfo s_WifiInfo = {"", ""};


enum WIFI_SETUP_ESP
{
  ESP_NO_SETUP = 0,
  ESP_SETUP_CLIENT_CONNECT,
  ESP_HAS_SETUP,
  ESP_IS_CONNECTED
};

WiFiClient client;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html style="height: 100%; width: 100%;">
    <head>
        <meta name="viewport" content="width=device-width, initial-scale=1">
    </head>
    <body style="height: 100%; width: 100%; overflow: hidden;">
        <div style="height: 100%; width: 100%; padding: 10px; display: flex;flex-direction: column;align-items: center;">
            <h1>Configuração BMS Titânium</h1>
            <form id="setupForm" action="/setup" method="post">
                <label for="ssid">Nome da Rede:</label><br>
                <input type="text" id="ssid" name="ssid"><br>
                <label for="pass">Senha:</label><br>
                <input type="password" id="pass" name="pass"><br><br>
                <input type="submit" name="Submit">
            </form> 
            <iframe id="ifrm1" name="ifrm1" style="display:none"></iframe>
        </div>
    </body>
</html>)rawliteral";

const char configured_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html style="height: 100%; width: 100%;">
    <head>
        <meta name="viewport" content="width=device-width, initial-scale=1">
    </head>
    <body style="height: 100%; width: 100%; overflow: hidden;">
        <div style="height: 100%; width: 100%; padding: 10px; display: flex;flex-direction: column;align-items: center;">
            <h1>BMS configurado!</h1>
        </div>
    </body>
</html>)rawliteral";

WIFI_SETUP_ESP m_SetupState = ESP_NO_SETUP;
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

void ReadClientMessage(String &outString)
{
  unsigned int count = client.available();
  while (count > 0)
  {
    char c = client.read();
    outString += c;
    count--;
  }

  if (outString != "")
  {
    Serial.println("Recebido: " + outString);
  }
}

void HtmlPageSend()
{
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println("Connection: close");
  client.println();
  client.println(index_html);
  client.println();
}

void HtmlPageSucessSend()
{
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println("Connection: close");
  client.println();
  client.println(configured_html);
  client.println();
}

String GetStringUntilSeparator(String& inString, int beginIndex, char separator)
{
  char c;
  int count = beginIndex;
  String out = "";
  while(count < inString.length())
  {
    c = inString[count];
    if(c == separator)
    {
      break;
    }

    out += c;
    count++;
  }

  return out;
}

int ClientMessageHandler(String &httpMessage)
{
  if (httpMessage.indexOf("POST /setup") >= 0)
  {
    Serial.println("\nPost message\n");
    int indexOfMessage = httpMessage.indexOf("\r\n\r\n");
    if (indexOfMessage >= 0)
    {
      String newSsid = "";
      String newPassword = "";

      int ssidIndex = httpMessage.indexOf('ssid=', indexOfMessage);

      if (ssidIndex >= 0)
      {
        newSsid = GetStringUntilSeparator(httpMessage, ssidIndex, '&');
        unsigned int newPasswordIndex = ssidIndex + 6 + newSsid.length();
        newPassword = GetStringUntilSeparator(httpMessage, newPasswordIndex, '&');
      }
      
      if(newSsid != "" && newPassword != "")
      {
        s_WifiInfo.s_Password = newPassword;
        s_WifiInfo.s_Ssid = newSsid;
        HtmlPageSucessSend();
        return 1;
      }
    }
  }
  else if (httpMessage.indexOf("GET /") >= 0)
  {
    HtmlPageSend();
  }
  return 0;
}

void EspSetup()
{
  server.close();
  WiFi.softAPdisconnect();
  WiFi.mode(WIFI_MODE_STA);

  Serial.println(WiFi.begin(s_WifiInfo.s_Ssid.c_str(), s_WifiInfo.s_Password.c_str()));
}

void SetupLoop()
{
  int disconnect = 0;
  switch (m_SetupState)
  {
  case ESP_NO_SETUP:
    client = server.available();
    if (client)
    {
      m_SetupState = ESP_SETUP_CLIENT_CONNECT;
      // HtmlPageSend();
      Serial.println("Conectado");
    }
    // Serial.println("Aqui");

    break;

  case ESP_SETUP_CLIENT_CONNECT:
   
    if (!client.connected())
    {
      disconnect = 1;
    }
    else if (client.available() > 0)
    {
      String httpMessage;
      ReadClientMessage(httpMessage);
      if(ClientMessageHandler(httpMessage))
      {
        Serial.println("Setup configurado");
        m_SetupState = ESP_HAS_SETUP;
        client.stop();
        EspSetup();
      }
      else
      {
        disconnect = 1;
      }
    }
    if (disconnect > 0)
    {
      m_SetupState = ESP_NO_SETUP;
      Serial.println("Desconectado");
      client.stop();
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
      m_SetupState = ESP_NO_SETUP;
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
