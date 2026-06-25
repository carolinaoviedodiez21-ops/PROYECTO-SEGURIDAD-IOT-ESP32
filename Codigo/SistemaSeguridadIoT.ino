
// LIBRERÍAS

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <ThingSpeak.h>
#include <UniversalTelegramBot.h>
#include <BluetoothSerial.h>

// WIFI

const char* ssid = "TU_WIFI_NOMBRE";
const char* password = "TU_CLAVE";

// TELEGRAM

#define BOT_TOKEN "TU_TOKEN"
#define CHAT_ID "TU_CHAT_ID"

WiFiClientSecure secureClient;
UniversalTelegramBot bot(BOT_TOKEN, secureClient);

// THINGSPEAK


WiFiClient client;

unsigned long channelID = 0;
const char* writeAPIKey = "TU_API_KEY"
const char* password = "TU_PASSWORD"


// BLUETOOTH

BluetoothSerial SerialBT;

// PINES

#define TRIG_PIN      19
#define ECHO_PIN      18

#define PIR_PIN       33

#define LED_ROJO      14
#define LED_BLANCO_1  16
#define LED_BLANCO_2  17

#define BUZZER_PIN    15

// VARIABLES

bool sistemaArmado = false;   // inicia desarmado

bool alertaPuerta = false;
bool intrusionConfirmada = false;

bool telegramPuertaEnviado = false;
bool telegramIntrusionEnviado = false;

unsigned long tiempoInicioPuerta = 0;
unsigned long tiempoAlerta = 0;

unsigned long ultimoParpadeo = 0;
bool estadoParpadeo = false;

// CONFIGURACIÓN

const float DISTANCIA_UMBRAL = 10.0;

const unsigned long TIEMPO_PUERTA = 5000;
const unsigned long TIEMPO_CONFIRMACION = 30000;

// MEDIR DISTANCIA

float medirDistancia() {

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  long duracion = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duracion == 0)
    return -1;

  return duracion * 0.034 / 2.0;
}

// SETUP

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

  noTone(BUZZER_PIN);

  // Bluetooth
  SerialBT.begin("SEGURIDAD_IOT");

  // WiFi
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado");

  secureClient.setInsecure();

  ThingSpeak.begin(client);

  Serial.println("Sistema iniciado");
}

// LOOP

void loop() {

  // BLUETOOTH enciende el sistema

  if (SerialBT.available()) {

    char comando = SerialBT.read();

    if (comando == '1') {

      sistemaArmado = true;

      bot.sendMessage(
        CHAT_ID,
        "Sistema armado desde App Inventor",
        "");

      Serial.println("Sistema ACTIVADO");
    }

    if (comando == '0') {

      sistemaArmado = false;

      alertaPuerta = false;
      intrusionConfirmada = false;

      telegramPuertaEnviado = false;
      telegramIntrusionEnviado = false;

      tiempoInicioPuerta = 0;

      digitalWrite(LED_ROJO, LOW);
      digitalWrite(LED_BLANCO_1, LOW);
      digitalWrite(LED_BLANCO_2, LOW);

      noTone(BUZZER_PIN);

      bot.sendMessage(
        CHAT_ID,
        "Sistema desarmado desde App Inventor",
        "");

      Serial.println("Sistema DESACTIVADO");
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

    delay(100);
    return;
  }

  
  // ESTADO NORMAL el sistema activo enciended luz roja 


  if (!alertaPuerta && !intrusionConfirmada) {

    digitalWrite(LED_ROJO, HIGH);

    digitalWrite(LED_BLANCO_1, LOW);
    digitalWrite(LED_BLANCO_2, LOW);

    noTone(BUZZER_PIN);

    if (distancia > 0 &&
        distancia < DISTANCIA_UMBRAL) {

      if (tiempoInicioPuerta == 0) {
        tiempoInicioPuerta = millis();
      }

      if (millis() - tiempoInicioPuerta >= TIEMPO_PUERTA) {

        alertaPuerta = true;
        tiempoAlerta = millis();

        if (!telegramPuertaEnviado) {

          bot.sendMessage(
            CHAT_ID,
            "Presencia detectada en puerta",
            "");

          telegramPuertaEnviado = true;
        }
      }
    }
    else {

      tiempoInicioPuerta = 0;
    }
  }

  // ==================================================
  // ALERTA DE PUERTA la luz roja parpadea y las blancas internas se encienden con el fin de disuadir
  

  if (alertaPuerta && !intrusionConfirmada) {

    if (millis() - ultimoParpadeo >= 500) {

      estadoParpadeo = !estadoParpadeo;

      digitalWrite(LED_ROJO, estadoParpadeo);

      ultimoParpadeo = millis();
    }

    digitalWrite(LED_BLANCO_1, HIGH);
    digitalWrite(LED_BLANCO_2, HIGH);

    if (movimiento) {

      intrusionConfirmada = true;

      if (!telegramIntrusionEnviado) {

        bot.sendMessage(
          CHAT_ID,
          "INTRUSION CONFIRMADA",
          "");

        telegramIntrusionEnviado = true;
      }
    }

    if ((millis() - tiempoAlerta) >= TIEMPO_CONFIRMACION) {

      alertaPuerta = false;

      tiempoInicioPuerta = 0;

      telegramPuertaEnviado = false;

      digitalWrite(LED_BLANCO_1, LOW);
      digitalWrite(LED_BLANCO_2, LOW);

      digitalWrite(LED_ROJO, HIGH);

      Serial.println("Falsa alarma");
    }
  }

 
  // INTRUSIÓN CONFIRMADA si hay intruso el PIR interno lo dectecta y se activa alerta sonoro-luminica
 

  if (intrusionConfirmada) {

    if (millis() - ultimoParpadeo >= 200) {

      estadoParpadeo = !estadoParpadeo;

      digitalWrite(LED_ROJO, estadoParpadeo);
      digitalWrite(LED_BLANCO_1, estadoParpadeo);
      digitalWrite(LED_BLANCO_2, estadoParpadeo);

      if (estadoParpadeo) {
        tone(BUZZER_PIN, 1500);
      }
      else {
        tone(BUZZER_PIN, 2500);
      }

      ultimoParpadeo = millis();
    }
  }

  
  // THINGSPEAK guarda los datos
 

  ThingSpeak.setField(1, distancia);
  ThingSpeak.setField(2, movimiento);
  ThingSpeak.setField(3, intrusionConfirmada);
  ThingSpeak.setField(4, sistemaArmado);

  ThingSpeak.writeFields(channelID, writeAPIKey);


  // MONITOR SERIE


  Serial.print("Distancia: ");
  Serial.print(distancia);

  Serial.print(" cm | PIR: ");
  Serial.print(movimiento);

  Serial.print(" | Alerta puerta: ");
  Serial.print(alertaPuerta);

  Serial.print(" | Intrusion: ");
  Serial.print(intrusionConfirmada);

  Serial.print(" | Armado: ");
  Serial.println(sistemaArmado);

  delay(100);
}
