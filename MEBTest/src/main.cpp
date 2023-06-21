#include <Arduino.h>

#include <array>
#include <vector>

#include <Adafruit_NeoPixel.h>
#include <Adafruit_BMP280.h>

#if defined(ARDUINO_ARCH_ESP32) && !defined(CONFIG_IDF_TARGET_ESP32C3)
const uint8_t PIN_LED1 = 2;
const uint8_t PIN_RGB = 13;

const uint8_t PIN_BTN1 = 34;
const uint8_t PIN_BTN2 = 35;

const uint8_t PIN_SDA = 21;
const uint8_t PIN_SCL = 21;

#elif defined(CONFIG_IDF_TARGET_ESP32C3)
const uint8_t PIN_LED1 = 2;
const uint8_t PIN_RGB = 8;

const uint8_t PIN_BTN1 = 6;
const uint8_t PIN_BTN2 = 7;

const uint8_t PIN_SDA = 4;
const uint8_t PIN_SCL = 5;

#elif defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_MBED_RP2040)
const uint8_t PIN_LED1 = 18;
const uint8_t PIN_RGB = 19;

const uint8_t PIN_BTN1 = 20;
const uint8_t PIN_BTN2 = 21;

const uint8_t PIN_SDA = 4;
const uint8_t PIN_SCL = 5;

#elif defined(ARDUINO_ARCH_STM32)
const uint8_t PIN_LED1 = PB13;
const uint8_t PIN_RGB = PB12;

const uint8_t PIN_BTN1 = PB14;
const uint8_t PIN_BTN2 = PB15;

const uint8_t PIN_SDA = PB7;
const uint8_t PIN_SCL = PB6;
#endif

typedef std::array<uint8_t, 3> Colour;
const std::vector<Colour> colours = {
  {0, 0, 0},
  {255, 255, 255},
  {255, 0, 0},
  {255, 255, 0},
  {0, 255, 0},
  {0, 255, 255},
  {0, 0, 255},
  {255, 0, 255},
};

Adafruit_NeoPixel onboard_neopixel(1, PIN_RGB, NEO_GRB + NEO_KHZ800);
Adafruit_BMP280 i2c_bmp280;

const bool BTN_DOWN = LOW;
const bool BTN_UP = HIGH;

int step;
int led_step;
int rgb_step;
bool btn1_last;
bool btn2_last;

void setup() {
  Serial.begin(115200);

  pinMode(PIN_LED1, OUTPUT);
  pinMode(PIN_RGB, OUTPUT);

  pinMode(PIN_BTN1, INPUT_PULLUP);
  pinMode(PIN_BTN2, INPUT_PULLUP);

  onboard_neopixel.begin();

  i2c_bmp280.begin(0x76);

  step = 0;
  led_step = 0;
  rgb_step = 0;
  btn1_last = BTN_UP;
  btn2_last = BTN_UP;
}

void loop() {
  const bool btn1 = digitalRead(PIN_BTN1);
  if (btn1_last == BTN_DOWN && btn1 == BTN_UP) led_step = (led_step + 1) % 2;
  btn1_last = btn1;

  const bool btn2 = digitalRead(PIN_BTN2);
  if (btn2_last == BTN_DOWN && btn2 == BTN_UP) rgb_step = (rgb_step + 1) % colours.size();
  btn2_last = btn2;

  digitalWrite(PIN_LED1, led_step % 2);

  onboard_neopixel.clear();
  const Colour &colour = colours[rgb_step % colours.size()];
  onboard_neopixel.setPixelColor(0, onboard_neopixel.Color(colour[0], colour[1], colour[2]));
  onboard_neopixel.setBrightness(31);
  onboard_neopixel.show();

  Serial.println(String("BTN1: ") + String(digitalRead(PIN_BTN1) ? "HIGH" : "LOW"));
  Serial.println(String("BTN2: ") + String(digitalRead(PIN_BTN2) ? "HIGH" : "LOW"));

  Serial.println(String("I2C Temp: ") + String(i2c_bmp280.readTemperature()));

  delay(100);

  step++;
}
