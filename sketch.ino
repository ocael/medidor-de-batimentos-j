#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <PubSubClient.h>

// ----- OLED -----
#define TELA_W 128
#define TELA_H 64
#define TELA_RST -1
Adafruit_SSD1306 oled(TELA_W, TELA_H, &Wire, TELA_RST);

// ----- PINOS -----
#define SENSOR_PULSO 35
#define LED_AMAR 2      // abaixo do limite
#define LED_VERM 5      // acima do limite

// ----- Limites de batimentos -----
int faixaMin = 50;
int faixaMax = 120;

// ----- WiFi / MQTT -----
const char* ssid = "Wokwi-GUEST";
const char* pass = "";
const char* broker = "test.mosquitto.org";

WiFiClient espClient;
PubSubClient mqtt(espClient);

// -------------------------------------------------------------------
//  Função: zerar LEDs
// -------------------------------------------------------------------
void apagarIndicadores() {
  digitalWrite(LED_AMAR, LOW);
  digitalWrite(LED_VERM, LOW);
}

// -------------------------------------------------------------------
//  Conectar WiFi
// -------------------------------------------------------------------
void conectarWiFi() {
  WiFi.begin(ssid, pass);
  Serial.print("Conectando ao WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }

  Serial.println("\nWiFi conectado!");
}

// -------------------------------------------------------------------
//  Reconectar MQTT
// -------------------------------------------------------------------
void conectarMQTT() {
  while (!mqtt.connected()) {
    Serial.println("Reconectando ao servidor MQTT...");
    if (mqtt.connect("esp32_monitorBPM")) {
      Serial.println("MQTT conectado!");
    } else {
      Serial.println("Falha, tentando novamente...");
      delay(1000);
    }
  }
}

// -------------------------------------------------------------------
//  Setup
// -------------------------------------------------------------------
void setup() {
  Serial.begin(115200);

  pinMode(LED_AMAR, OUTPUT);
  pinMode(LED_VERM, OUTPUT);

  apagarIndicadores();

  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Falha ao iniciar o display!");
    while (true);
  }

  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 25);
  oled.println("Iniciando Monitor...");
  oled.display();
  delay(1500);

  // ----- Liga WiFi e MQTT -----
  conectarWiFi();
  mqtt.setServer(broker, 1883);
}

// -------------------------------------------------------------------
//  Loop Principal
// -------------------------------------------------------------------
void loop() {

  if (!mqtt.connected()) {
    conectarMQTT();
  }
  mqtt.loop();

  // Leitura do sensor e conversão para BPM aproximado
  int valorSensor = analogRead(SENSOR_PULSO);
  int ritmo = map(valorSensor, 300, 600, 40, 180);
  ritmo = constrain(ritmo, 0, 200);

  apagarIndicadores();

  oled.clearDisplay();
  oled.setCursor(0, 0);

  // Estado do batimento
  if (ritmo < faixaMin) {
    oled.println("Baixo Desempenho");
    oled.setCursor(0, 18);
    oled.print("Leitura: ");
    oled.print(ritmo);
    oled.println(" bpm");
    oled.display();

    digitalWrite(LED_AMAR, HIGH);

    Serial.println("Status detectado: nivel inferior");

    mqtt.publish("monitorBPM/estado", "baixo");
  }
  else if (ritmo > faixaMax) {
    oled.println("Pulso Elevado");
    oled.setCursor(0, 18);
    oled.print("Leitura: ");
    oled.print(ritmo);
    oled.println(" bpm");
    oled.display();

    digitalWrite(LED_VERM, HIGH);

    Serial.println("Status detectado: nivel alto");

    mqtt.publish("monitorBPM/estado", "alto");
  }
  else {
    oled.println("Ritmo Adequado");
    oled.setCursor(0, 18);
    oled.print("Leitura: ");
    oled.print(ritmo);
    oled.println(" bpm");
    oled.display();

    Serial.println("Status detectado: dentro da faixa");

    mqtt.publish("monitorBPM/estado", "normal");
  }

  Serial.print("Medida atual: ");
  Serial.println(ritmo);

  // envia valor no MQTT
  mqtt.publish("monitorBPM/valor", String(ritmo).c_str());

  delay(1800);
}
