#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>
#include <FS.h>
#include "secrets.h"
/*// Configura los detalles de tu red WiFi
const char* ssid = "TU_SSID";
const char* password = "TU_PASSWORD";
en secrets.h
*/
// Inicializa el servidor WebSocket en el puerto 81
WebSocketsServer webSocket = WebSocketsServer(81);
WiFiServer server(80);
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
const int ledPin = LED_BUILTIN; // Pin al que está conectado el LED

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW); // LED apagado inicialmente

  // Conecta a la red WiFi
  WiFi.begin(ssid, password);
  Serial.println("Conectando a WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi conectado.");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());

  // Inicia el servidor WebSocket
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  // Inicia SPIFFS
  if (!SPIFFS.begin()) {
    Serial.println("Error al montar SPIFFS");
    return;
  }

  // Inicia el servidor web
  server.begin();
}

void loop() {
  webSocket.loop();
  WiFiClient client = server.available();
  if (client) {
    Serial.println("Nuevo cliente");
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        if (c == '\n') {
          if (currentLine.length() == 0) {
            // Sirve el archivo HTML desde SPIFFS
            File file = SPIFFS.open("/index.html", "r");
            if (!file) {
              Serial.println("Error al abrir index.html");
              client.print("HTTP/1.1 500 Internal Server Error\r\n");
              client.print("Content-Type: text/plain\r\n\r\n");
              client.print("500 Internal Server Error");
            } else {
              client.print("HTTP/1.1 200 OK\r\n");
              client.print("Content-Type: text/html\r\n\r\n");
              while (file.available()) {
                client.write(file.read());
              }
              file.close();
            }
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    client.stop();
    Serial.println("Cliente desconectado");
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_TEXT) {
    String msg = String((char *)payload).substring(0, length);
    Serial.println("Mensaje recibido: " + msg);
    if (msg == "ON") {
      digitalWrite(ledPin, HIGH); // Enciende el LED
      webSocket.sendTXT(num, "LED ON");
    } else if (msg == "OFF") {
      digitalWrite(ledPin, LOW); // Apaga el LED
      webSocket.sendTXT(num, "LED OFF");
    }
  }
}
