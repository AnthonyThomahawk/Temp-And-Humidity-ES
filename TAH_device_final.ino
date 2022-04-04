#include "DHT.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,20,4);

#define ATPIN 2

#define DHTPIN 5 // DHT sensor pin 2

#define THERMISTORPIN 1 // This is an Analog pin

#define DHTTYPE DHT11

#define L1 13 // LED 1 pin 13

#define S1 4 // Button 1 pin 4 : shows TMAX

#define S2 3 // Button 2 pin 3 : Hardware Reset

#define S3 2 // Button 3 pin 2 : Toggles backlight

#define RESET_PIN 9

DHT dht(DHTPIN, DHTTYPE);

float tMAX = -9999;
bool backlight_state;

unsigned long lastdebounce = 0;
unsigned long deboucne_delay_ms = 200;

unsigned long LED_ms = 1000;
unsigned long Backlight_toggle_ms = 100;

unsigned long lastled = 0;
unsigned long lastbacklight = 0;

void setup()
{
  digitalWrite(RESET_PIN, HIGH);
  pinMode(RESET_PIN, OUTPUT);
  pinMode(L1, OUTPUT);
  pinMode(S1, INPUT_PULLUP);
  pinMode(S2, INPUT_PULLUP);
  pinMode(S3, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(S2), resetInterrupt, CHANGE); // interrupt for Reset
  
  lcd.init(); // start LCD display

  lcd.backlight(); // turn on backlight
  backlight_state = true;
  Serial.begin(9600);

  lcd.setCursor(1,0);
  lcd.print("Starting...");

  dht.begin();
}

// formula and code from :
// https://www.jameco.com/z/NTC-103-R-Thermistor-NTC-K-10-10k-Ohm_207037.html
// modified Beta value according to my thermistor from https://product.tdk.com/system/files/dam/doc/product/sensor/ntc/ntc_assy/catalog/sensor_ntc-thermistor_assembly_en.pdf?ref_disty=mouser
float readAnalogTemp()
{
    const float invBeta = 1.00 / 3988.00;   // replace "Beta" with beta of thermistor
  
    const  float adcMax = 1023.00;
    const float invT0 = 1.00 / 298.15;   // room temp in Kelvin
  
    int adcVal, i, numSamples = 5;
    float  K, C;
  
    adcVal = 0;
    for (i = 0; i < numSamples; i++)
    {
      adcVal = adcVal + analogRead(THERMISTORPIN);
      delay(100);
    }
    adcVal = adcVal / 5;
    K = 1.00 / (invT0 + invBeta * (log ( adcMax / (float) adcVal - 0.95)));
    C = K - 273.15; // convert to Celsius
    return C;
}

void loop()
{
  if (timer(lastled, LED_ms))
  {
    digitalWrite(L1, HIGH); // blink LED
    delay(300);
    digitalWrite(L1, LOW);
  }
  if (timer(lastbacklight, Backlight_toggle_ms) && digitalRead(S3) == LOW)
  {
      if (backlight_state)
      {
        lcd.noBacklight();
      }
      else
      {
        lcd.backlight();
      }
  
      backlight_state = !backlight_state;
  }
 
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float analogt = readAnalogTemp();

  if (t > tMAX) { tMAX = t; }

  if (isnan(h) || isnan(t)) { // if h or t are null, dont print on LCD
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  lcd.clear();

  lcd.setCursor(0,0); // change cursor on LCD display (columns,rows)
  lcd.print("AM : REDACTED");
  lcd.setCursor(0,1);
  lcd.print("DT=" + String(t) + "C " + "AT=" + String(analogt) + "C");
  lcd.setCursor(0,2);
  lcd.print("Humidity = " + String(h) + "%");

  if (digitalRead(S1) == LOW) // if S1 is held down, print max temp
  {
    lcd.setCursor(0,3);
    lcd.print("tM=" + String(tMAX) + "C " + "aV="+ String(analogRead(THERMISTORPIN))); // print max temp from DHT11 and analog reading of thermistor
  }
  
}

bool timer(unsigned long & targetms, unsigned long threshold)
{
  if (targetms == 0 || (millis() - targetms) > threshold)
  {
    targetms = millis();
    return true;
  }
  return false;
}

void resetInterrupt()
{
  digitalWrite(RESET_PIN, LOW); // hardware reset with interrupt
}
