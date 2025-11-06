#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// --- CONFIGURACIÓN OLED ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_SDA 4       // D2
#define OLED_SCL 5       // D1
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- CONFIGURACIÓN WiFi ---
const char* ssid = "INFINITUM19C0";
const char* password = "Jg6Ws9Je3t";

// --- CONFIGURACIÓN MQTT ---
const char* mqtt_server = "test.mosquitto.org";
const int mqtt_port = 1883;
const char* mqtt_client_id = "ESP8266_Vehiculos";
const char* mqtt_username = "";
const char* mqtt_password = "";
const char* topic_subs = "vehiculos/conteo"; // Tema al que se suscribe

WiFiClient wifiClient;
PubSubClient client(wifiClient);

// --- VARIABLES ---
int lugares_autos = 0;
int lugares_motos = 0;
int lugares_camiones = 0;

// -----------------------------------------------------------
// CALLBACK: Se ejecuta cuando llega un mensaje MQTT
// -----------------------------------------------------------
void callbackMQTT(char* topic, byte* payload, unsigned int length) {
  String mensaje = "";
  for (unsigned int i = 0; i < length; i++) {
    mensaje += (char)payload[i];
  }

  Serial.print("Mensaje recibido [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(mensaje);

  // Espera formato: "autos,motos,camiones" (ej. "4,2,1")
  int primerComa = mensaje.indexOf(',');
  int segundaComa = mensaje.indexOf(',', primerComa + 1);

  if (primerComa > 0 && segundaComa > primerComa) {
    lugares_autos = mensaje.substring(0, primerComa).toInt();
    lugares_motos = mensaje.substring(primerComa + 1, segundaComa).toInt();
    lugares_camiones = mensaje.substring(segundaComa + 1).toInt();

    Serial.print("Lugares Autos: ");
    Serial.println(lugares_autos);
    Serial.print("Lugares Motos: ");
    Serial.println(lugares_motos);
    Serial.print("Lugares Camiones: ");
    Serial.println(lugares_camiones);

    // Mostrar en OLED
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Lugares disponibles:");
    display.setCursor(0, 15);
    display.print("Autos: ");
    display.println(lugares_autos);
    display.setCursor(0, 30);
    display.print("Motos: ");
    display.println(lugares_motos);
    display.setCursor(0, 45);
    display.print("Camiones: ");
    display.println(lugares_camiones);
    display.display();
  } else {
    Serial.println("⚠️ Formato incorrecto del mensaje MQTT");
  }
}

// -----------------------------------------------------------
// Conexión a WiFi
// -----------------------------------------------------------
void connectWiFi() {
  Serial.print("Conectando a WiFi: ");
  Serial.println(ssid);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.println("Conectando WiFi...");
  display.display();

  WiFi.begin(ssid, password);
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    retry++;
    if (retry > 60) { // 30 segundos
      Serial.println("\nError conectando WiFi. Reiniciando...");
      ESP.restart();
    }
  }

  Serial.println("\nWiFi Conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("WiFi conectado!");
  display.setCursor(0, 10);
  display.print("IP: ");
  display.println(WiFi.localIP());
  display.display();
  delay(1500);
}

// -----------------------------------------------------------
// Conexión a MQTT
// -----------------------------------------------------------
void ensureMQTT() {
  while (!client.connected()) {
    Serial.print("Conectando a MQTT...");
    if (client.connect(mqtt_client_id, mqtt_username, mqtt_password)) {
      Serial.println("Conectado a Mosquitto!");
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("MQTT conectado!");
      display.display();
      delay(1000);

      client.subscribe(topic_subs);
      Serial.print("Suscrito al tema: ");
      Serial.println(topic_subs);
    } else {
      Serial.print("Fallo rc=");
      Serial.print(client.state());
      Serial.println(" reintentando en 3s...");
      delay(3000);
    }
  }
}

// -----------------------------------------------------------
// SETUP
// -----------------------------------------------------------
void setup() {
  Serial.begin(115200);
  Wire.begin(OLED_SDA, OLED_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("No se encuentra la pantalla OLED");
    while (true);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Iniciando...");
  display.display();
  delay(1500);

  connectWiFi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callbackMQTT);
}

// -----------------------------------------------------------
// LOOP PRINCIPAL
// -----------------------------------------------------------
void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  ensureMQTT();
  client.loop();
}
