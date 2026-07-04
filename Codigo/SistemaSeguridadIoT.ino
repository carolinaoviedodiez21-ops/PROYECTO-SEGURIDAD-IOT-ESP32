// OVIEDO DIEZ CAROLINA DEL CARMEN
// PROYECTO DE PROGRAMACIÓN AVANZADA
// SISTEMA DE SEGURIDAD IoT CON ESP32

#include <WiFi.h>
#include <ThingSpeak.h>
#include <BluetoothSerial.h>
#include <ESP_Mail_Client.h>

// ================= WIFI =================
const char* ssid = "InternetIoT"; //"Personal-WiFi-7E6-2.4Ghz";
const char* password = "Carolina21"; //"JTwswJ2ASA";

// ================= THINGSPEAK =================
unsigned long channelID = 3408842;
const char* writeAPIKey = "V1JLNQ93XKS4DD74";

WiFiClient tsClient;

// ================= BLUETOOTH =================
BluetoothSerial SerialBT;

// ================= EMAIL (ESP MAIL CLIENT) =================
SMTPSession smtp;
Session_Config config;

bool emailEnviado = false;

// ================= PINES =================
#define TRIG_PIN 32
#define ECHO_PIN 35
#define PIR_PIN 33

#define LED_ROJO 14
#define LED_BLANCO_1 26
#define LED_BLANCO_2 27

#define BUZZER_PIN 12

// ================= VARIABLES =================
bool sistemaArmado = true;
bool alertaPuerta = false;
bool intrusionConfirmada = false;

unsigned long tiempoInicioPuerta = 0;
unsigned long tiempoAlerta = 0;

unsigned long ultimoParpadeo = 0;
unsigned long ultimoEnvioTS = 0;

bool estadoParpadeo = false;

// ================= CONSTANTES =================
const float DISTANCIA_UMBRAL = 10.0;
const unsigned long TIEMPO_PUERTA = 5000;
const unsigned long TIEMPO_CONFIRMACION = 15000;

// ================= DISTANCIA =================
float medirDistancia() {

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  long duracion = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duracion == 0) return -1;

  return duracion * 0.034 / 2.0;
}

// ================= ENVÍO EMAIL =================
void enviarCorreoIntrusion() {

  config.server.host_name = "smtp.gmail.com";
  config.server.port = 465;
  config.login.email = "carolinaoviedodiez21@gmail.com";
  config.login.password = "mmqxypksioatpirf";
  config.login.user_domain = "";

  smtp.debug(1);
  smtp.callback(NULL);

  SMTP_Message message;

  message.sender.name = "Sistema IoT";
  message.sender.email = config.login.email;

  message.subject = "ALERTA - INTRUSIÓN DETECTADA";

  message.addRecipient("Usuario", config.login.email);

  String texto =
    "ALERTA DE SEGURIDAD\n\n"
    "Se detectó una intrusión en el sistema IoT.\n"
    "Revise inmediatamente la instalación.";

  message.text.content = texto.c_str();
  message.text.charSet = "utf-8";
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

  if (!smtp.connect(&config)) {
    Serial.println("Error SMTP connect");
    return;
  }

  if (!MailClient.sendMail(&smtp, &message)) {
    Serial.println("Error enviando email");
  } else {
    Serial.println("Email enviado correctamente");
  }
}

// ================= SETUP =================
void setup() {

  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);

  pinMode(LED_ROJO, OUTPUT);
  pinMode(LED_BLANCO_1, OUTPUT);
  pinMode(LED_BLANCO_2, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(LED_ROJO, LOW);
  digitalWrite(LED_BLANCO_1, LOW);
  digitalWrite(LED_BLANCO_2, LOW);

  SerialBT.begin("SEGURIDAD_IOT");

  // WIFI
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado");

  ThingSpeak.begin(tsClient);
}

// ================= LOOP =================
void loop() {

  // Bluetooth control
  if (SerialBT.available()) {
    char c = SerialBT.read();

    if (c == '1') sistemaArmado = true;
    if (c == '0') {
      sistemaArmado = false;
      alertaPuerta = false;
      intrusionConfirmada = false;
      emailEnviado = false;
    }
  }

  float distancia = medirDistancia();
  bool movimiento = digitalRead(PIR_PIN);

  // SISTEMA DESARMADO
  if (!sistemaArmado) {
    digitalWrite(LED_ROJO, LOW);
    digitalWrite(LED_BLANCO_1, LOW);
    digitalWrite(LED_BLANCO_2, LOW);
    noTone(BUZZER_PIN);
    return;
  }

  // ESTADO NORMAL
  if (!alertaPuerta && !intrusionConfirmada) {

    digitalWrite(LED_ROJO, HIGH);

    if (distancia > 0 && distancia < DISTANCIA_UMBRAL) {

      if (tiempoInicioPuerta == 0)
        tiempoInicioPuerta = millis();

      if (millis() - tiempoInicioPuerta >= TIEMPO_PUERTA) {
        alertaPuerta = true;
        tiempoAlerta = millis();
      }

    } else {
      tiempoInicioPuerta = 0;
    }
  }

  // ALERTA PUERTA
  if (alertaPuerta && !intrusionConfirmada) {

    digitalWrite(LED_BLANCO_1, HIGH);
    digitalWrite(LED_BLANCO_2, HIGH);

    if (movimiento && !emailEnviado) {
      intrusionConfirmada = true;
      enviarCorreoIntrusion();
      emailEnviado = true;
    }
  }

  // INTRUSIÓN
  if (intrusionConfirmada) {

    if (millis() - ultimoParpadeo > 200) {
      estadoParpadeo = !estadoParpadeo;

      digitalWrite(LED_ROJO, estadoParpadeo);
      digitalWrite(LED_BLANCO_1, estadoParpadeo);
      digitalWrite(LED_BLANCO_2, estadoParpadeo);

      tone(BUZZER_PIN, estadoParpadeo ? 3000 : 1500);

      ultimoParpadeo = millis();
    }
  }

  // THINGSPEAK
  if (millis() - ultimoEnvioTS > 20000) {

    ThingSpeak.setField(1, distancia);
    ThingSpeak.setField(2, movimiento);
    ThingSpeak.setField(3, sistemaArmado);
    ThingSpeak.setField(4, intrusionConfirmada);

    ThingSpeak.writeFields(channelID, writeAPIKey);

    ultimoEnvioTS = millis();
  }


  // MONITOR SERIE
  

  Serial.print("Distancia: ");
  Serial.print(distancia);

  Serial.print(" cm | PIR: ");
  Serial.print(movimiento);

  Serial.print(" | Alerta puerta: ");
  Serial.print(alertaPuerta);

  Serial.print(" | Intrusion: ");
  Serial.println(intrusionConfirmada);

  Serial.print(" | Armado: ");
  Serial.println(sistemaArmado);

  delay(100);
}
