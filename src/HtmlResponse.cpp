#include "HtmlResponse.h"

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

void HtmlPageSucessSend(WiFiClient &client)
{
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html");
    client.println("Connection: close");
    client.println();
    client.println(configured_html);
    client.println();
}

void HtmlMainPageSend(WiFiClient &client)
{
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html");
    client.println("Connection: close");
    client.println();
    client.println(index_html);
    client.println();
}