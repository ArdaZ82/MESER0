/*
 * ============================================================
 *  DIAGNOSTICO — ROBOT MESERO (ESP32-S3 N16R8)
 * ============================================================
 *  Prueba cada parte del robot POR SEPARADO y reporta el
 *  resultado en el Monitor Serie (velocidad 115200).
 *
 *  COMO USARLO:
 *   1. Abre este sketch en el IDE de Arduino.
 *   2. Placa: "ESP32S3 Dev Module".  Puerto: el de tu ESP32.
 *   3. Sube el codigo (boton ->).
 *   4. Abre  Herramientas > Monitor Serie  y ponlo en 115200 baudios.
 *   5. Escribe un numero (1 a 5) en la barra de arriba del monitor
 *      y pulsa Enter para lanzar cada prueba:
 *        1 = Escaner I2C (busca las LCD)
 *        2 = Ultrasonidos (los 7)
 *        3 = Temperatura (DS18B20)
 *        4 = Motores (CUIDADO: giran; levanta las ruedas)
 *        5 = LCD (escribe texto en las 3 pantallas)
 *
 *  LIBRERIAS: Wire, OneWire, DallasTemperature, LiquidCrystal_I2C
 *  (las mismas del firmware final)
 * ============================================================
 */

#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C.h>

// ---- mismos pines que el firmware final ----
#define PIN_SDA 8
#define PIN_SCL 9
#define PIN_ONEWIRE 10

#define MI_RPWM 11
#define MI_LPWM 12
#define MD_RPWM 13
#define MD_LPWM 14

struct Ultra { uint8_t trig; uint8_t echo; const char* nombre; };
Ultra us[7] = {
  { 1,  2, "Frontal   "},
  { 4,  5, "Izquierdo "},
  { 6,  7, "Derecho   "},
  {15, 16, "Trasero   "},
  {17, 18, "Bandeja 1 "},
  {21, 38, "Bandeja 2 "},
  {39, 40, "Bandeja 3 "},
};

OneWire oneWire(PIN_ONEWIRE);
DallasTemperature ds(&oneWire);

// direcciones I2C esperadas de las 3 LCD
uint8_t dirLCD[3] = {0x27, 0x26, 0x25};

// ==================== PRUEBAS ====================

void pruebaI2C() {
  Serial.println("\n--- 1) ESCANER I2C ---");
  int encontrados = 0;
  for (uint8_t dir = 1; dir < 127; dir++) {
    Wire.beginTransmission(dir);
    if (Wire.endTransmission() == 0) {
      Serial.printf("  Dispositivo I2C en 0x%02X\n", dir);
      encontrados++;
    }
  }
  if (encontrados == 0)
    Serial.println("  NADA. Revisa SDA(8), SCL(9), VCC de 5V y GND comun.");
  else
    Serial.printf("  Total: %d dispositivo(s). Deberian salir 0x27, 0x26, 0x25.\n", encontrados);
}

void pruebaUltrasonidos() {
  Serial.println("\n--- 2) ULTRASONIDOS (5 lecturas c/u) ---");
  for (int i = 0; i < 7; i++) {
    Serial.printf("  %s: ", us[i].nombre);
    for (int n = 0; n < 5; n++) {
      digitalWrite(us[i].trig, LOW);  delayMicroseconds(3);
      digitalWrite(us[i].trig, HIGH); delayMicroseconds(10);
      digitalWrite(us[i].trig, LOW);
      unsigned long dur = pulseIn(us[i].echo, HIGH, 25000UL);
      if (dur == 0) Serial.print("--- ");
      else          Serial.printf("%.0fcm ", dur * 0.0343f / 2.0f);
      delay(60);
    }
    Serial.println();
  }
  Serial.println("  Si un sensor da siempre '---', revisa TRIG/ECHO y el divisor del ECHO.");
}

void pruebaTemperatura() {
  Serial.println("\n--- 3) TEMPERATURA (DS18B20) ---");
  ds.requestTemperatures();
  int n = ds.getDeviceCount();
  Serial.printf("  Sensores detectados en el bus: %d (esperados: 3)\n", n);
  for (int i = 0; i < n; i++) {
    float c = ds.getTempCByIndex(i);
    Serial.printf("  Sensor %d: %.1f C\n", i, c);
  }
  if (n == 0)
    Serial.println("  NADA. Revisa el pin 10, la resistencia pull-up de 4.7k a 3.3V y GND.");
}

void pulso(int pinA, int pinB, const char* etiqueta) {
  Serial.printf("  %s adelante...\n", etiqueta);
  ledcWrite(pinA, 160); ledcWrite(pinB, 0);
  delay(1500);
  ledcWrite(pinA, 0); ledcWrite(pinB, 0);
  delay(500);
  Serial.printf("  %s atras...\n", etiqueta);
  ledcWrite(pinA, 0); ledcWrite(pinB, 160);
  delay(1500);
  ledcWrite(pinA, 0); ledcWrite(pinB, 0);
}

void pruebaMotores() {
  Serial.println("\n--- 4) MOTORES ---");
  Serial.println("  !! LEVANTA LAS RUEDAS DEL SUELO ANTES DE CONTINUAR !!");
  Serial.println("  Empieza en 3 segundos...");
  delay(3000);
  pulso(MI_RPWM, MI_LPWM, "Motor IZQUIERDO");
  delay(800);
  pulso(MD_RPWM, MD_LPWM, "Motor DERECHO");
  Serial.println("  Listo. Si uno gira al reves, intercambia sus 2 cables en el BTS7960.");
}

void pruebaLCD() {
  Serial.println("\n--- 5) LCD ---");
  for (int i = 0; i < 3; i++) {
    LiquidCrystal_I2C lcd(dirLCD[i], 16, 2);
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0); lcd.print("Prueba LCD ");
    lcd.print(i + 1);
    lcd.setCursor(0, 1); lcd.print("Funciona OK!");
    Serial.printf("  Enviado texto a LCD %d (0x%02X)\n", i + 1, dirLCD[i]);
  }
  Serial.println("  Si una pantalla queda en blanco: ajusta su potenciometro o revisa su direccion.");
}

// ==================== SETUP / LOOP ====================
void setup() {
  Serial.begin(115200);
  delay(1500);

  Wire.begin(PIN_SDA, PIN_SCL);

  for (auto &u : us) {
    pinMode(u.trig, OUTPUT);
    pinMode(u.echo, INPUT);
    digitalWrite(u.trig, LOW);
  }

  ds.begin();

  ledcAttach(MI_RPWM, 15000, 8);
  ledcAttach(MI_LPWM, 15000, 8);
  ledcAttach(MD_RPWM, 15000, 8);
  ledcAttach(MD_LPWM, 15000, 8);

  Serial.println("\n==============================================");
  Serial.println("  DIAGNOSTICO ROBOT MESERO — listo.");
  Serial.println("  Escribe un numero y pulsa Enter:");
  Serial.println("    1=I2C  2=Ultrasonidos  3=Temp  4=Motores  5=LCD");
  Serial.println("==============================================");
}

void loop() {
  if (!Serial.available()) return;
  char c = Serial.read();
  switch (c) {
    case '1': pruebaI2C();          break;
    case '2': pruebaUltrasonidos(); break;
    case '3': pruebaTemperatura();  break;
    case '4': pruebaMotores();      break;
    case '5': pruebaLCD();          break;
    default:  return;   // ignora saltos de linea y otros caracteres
  }
  Serial.println("\n  Elige otra prueba (1-5):");
}
