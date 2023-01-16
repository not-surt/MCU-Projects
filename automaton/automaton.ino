#include <RF24.h>
#include <RF24_config.h>
#include <nRF24L01.h>
#include <printf.h>

#include <SPI.h>

#include "romote.h"

// Uncomment one of these
#define MOTOR_CONTROLLER_L298N_X1
// #define MOTOR_CONTROLLER_L298N_X2_MECANUM
// #define MOTOR_CONTROLLER_L293D

const uint8_t PWM_RES = 10u;
const uint16_t PWM_DUTY_MAX = (1u << PWM_RES) - 1u;
const uint16_t PWM_FREQ = 1000u;

struct Wheel {
  uint8_t index;
  uint8_t pinForward, pinReverse;
  bool isRear, isRight;
};

#if defined(ESP32) && defined(MOTOR_CONTROLLER_L298N_X1)
const uint8_t WHEEL_COUNT = 2;
Wheel wheels[] = {
  { 0u, 21u, 22u, false, false },  // Left
  { 1u, 25u, 23u, false, true },   // Right
};
#elif defined(ESP32) && defined(MOTOR_CONTROLLER_L298N_X2_MECANUM)
const uint8_t WHEEL_COUNT = 4;
Wheel wheels[] = {
  { 0u, 21u, 22u, false, false },  // Front-left
  { 1u, 25u, 23u, false, true },   // Front-right
  { 2u, 17u, 16u, true, false },   // Rear-left
  { 3u, 18u, 19u, true, true },    // Rear-right
};
#elif defined(ESP8266) && defined(MOTOR_CONTROLLER_L298N_X1)
const uint8_t WHEEL_COUNT = 2;
Wheel wheels[] = {
  { 0u, D2, D1, false, false },  // Left
  { 1u, D4, D3, false, true },   // Right
};
#elif defined(ESP8266) && defined(MOTOR_CONTROLLER_L293D)
const uint8_t WHEEL_COUNT = 2;
Wheel wheels[] = {
  { 0u, D1, D3, false, false },  // Left
  { 1u, D2, D4, false, true },   // Right
};
#endif

#if defined(ESP32)
const uint8_t RADIO_SCK = 14u, RADIO_MISO = 12u, RADIO_MOSI = 13u, RADIO_SS = 15u, RADIO_CE = 2u;
#elif defined(ESP8266)
const uint8_t RADIO_SCK = SCK, RADIO_MISO = MISO, RADIO_MOSI = MOSI, RADIO_SS = SS, RADIO_CE = 10u;
#endif
RF24 radio(RADIO_CE, RADIO_SS);
const uint8_t RADIO_CHANNEL = 0u;
const uint8_t RADIO_PIPE_ADDR[7] = "RoMote";
RadioPayload payload;
RadioPayload lastPayload;
unsigned long lastPayloadTime = 0ul;
unsigned long lastPayloadChangedTime = 0ul;

enum class DriveMode {
  Steer,
  Strafe
};
const int DRIVE_MODE_COUNT = 2;
DriveMode driveMode = DriveMode::Steer;

const int ANALOG_RES = 12;
const float ANALOG_MAX_VOLTAGE = 3.3f;
const int ANALOG_MAX = (1 << ANALOG_RES) - 1;
struct Battery {
  const uint8_t pin;
  const float measureMaxVoltage;
  const float reportMaxVoltage;
  const float pinMax;

  Battery(const uint8_t pin, const float measureMaxVoltage, const float reportMaxVoltage)
    : pin(pin), measureMaxVoltage(measureMaxVoltage), reportMaxVoltage(reportMaxVoltage), pinMax((float)ANALOG_MAX / ANALOG_MAX_VOLTAGE * measureMaxVoltage) {
  }
  float voltage() const {
    const int pinValue = analogRead(pin);
    return (float)pinValue / pinMax * reportMaxVoltage;
  }
};
float VOLTAGE_AVERAGE_WEIGHT = 0.0625f;
float voltageAverage;

uint32_t pwmFloatToInt(const float val) {
  return constrain((uint32_t)floor(0.5f + val * PWM_DUTY_MAX), 0u, PWM_DUTY_MAX);
}

void analogWriteFloat(const uint8_t pin, const uint8_t ch, const float val) {
  const uint32_t pwm = pwmFloatToInt(val);
#if defined(ESP32)
  ledcWrite(ch, pwm);
#elif defined(ESP8266) || defined(AVR) || defined(ARDUINO_ARCH_RP2040)
  analogWrite(pin, pwm);
#endif
}

struct Automaton {
  int lastMsgTime;
  Battery battery;

  Automaton()
    : lastMsgTime(0), battery(34, 2.4f, 8.4f) {
  }

  void setWheel(const Wheel &wheel, const float direction) const {
    // Serial.println("Direction: " + String(direction));
    const uint8_t chForward = wheel.index * 2u;
    const uint8_t chReverse = chForward + 1u;
    const float deadzone = 0.125f;
    const float duty = abs(direction);
    const bool reverse = direction < 0.0f;
#if defined(MOTOR_CONTROLLER_L298N_X1) || defined(MOTOR_CONTROLLER_L298N_X2_MECANUM)
    if (duty > deadzone) {
      analogWriteFloat(wheel.pinForward, chForward, reverse ? 0.0f : duty);
      analogWriteFloat(wheel.pinReverse, chReverse, reverse ? duty : 0.0f);
    } else {
      analogWriteFloat(wheel.pinForward, chForward, 0.0f);
      analogWriteFloat(wheel.pinReverse, chReverse, 0.0f);
    }
#elif defined(MOTOR_CONTROLLER_L293D)
    if (duty > deadzone) {
      Serial.println(String(wheel.index) + ": " + String(direction) + " = " + String(duty) + ", " + String(reverse));
      analogWriteFloat(wheel.pinForward, chForward, duty);
      digitalWrite(wheel.pinReverse, !reverse);
    } else {
      analogWriteFloat(wheel.pinForward, chForward, 0.0f);
      digitalWrite(wheel.pinReverse, 0u);
    }
#endif
  }

  void setDrive(const float drive = 0.0f, const float steer = 0.0f, const float strafe = 0.0f) const {
    for (uint8_t i = 0u; i < WHEEL_COUNT; ++i) {
      const Wheel &wheel = wheels[i];
      const float wheelStrafe = (wheel.isRight ^ wheel.isRear) ? -strafe : strafe;
      const float wheelSteer = wheel.isRight ? -steer : steer;
      setWheel(wheel, drive + wheelSteer + wheelStrafe);
    }
  }

  void init() {
#if defined(ESP8266)
    pinMode(1u, FUNCTION_3); // Enable TX as GPIO
    pinMode(3u, FUNCTION_3); // Enable RX as GPIO

    analogWriteResolution(PWM_RES);
    analogWriteFreq(PWM_FREQ);
#endif
#if defined(AVR)
    analogWriteResolution(PWM_RES);
    analogWriteFrequency(PWM_FREQ);
#endif

    for (uint8_t i = 0u; i < WHEEL_COUNT; ++i) {
      Wheel &wheel = wheels[i];
      pinMode(wheel.pinForward, OUTPUT);
      pinMode(wheel.pinReverse, OUTPUT);
#if defined(ESP32)
      const uint8_t chForward = wheel.index * 2u;
      const uint8_t chReverse = chForward + 1u;
      ledcSetup(chForward, PWM_FREQ, PWM_RES);
      ledcAttachPin(wheel.pinForward, chForward);
      ledcSetup(chReverse, PWM_FREQ, PWM_RES);
      ledcAttachPin(wheel.pinReverse, chReverse);
#elif defined(ESP8266) || defined(AVR)
      // digitalWrite(wheel.pinForward, HIGH);
      // digitalWrite(wheel.pinReverse, HIGH);
#endif
      yield();
    }

    voltageAverage = battery.voltage();

    pinMode(LED_BUILTIN, OUTPUT);

#if defined(ESP32)
    // analogSetAttenuation(ADC_11db);
    SPI.begin(RADIO_SCK, RADIO_MISO, RADIO_MOSI, RADIO_SS);  // SCK, MISO, MOSI, SS
#elif defined(ESP8266)
    SPI.begin();
#endif
    if (!radio.begin()) {
      Serial.println(F("Radio hardware not responding!"));
      while (true) {
        yield();
      }  // hold program in infinite loop to prevent subsequent errors
    } else {
      Serial.println(F("Radio hardware responding normally."));
    }
    radio.setPayloadSize(sizeof(RadioPayload));
    radio.setChannel(RADIO_CHANNEL);
    radio.openReadingPipe(1, RADIO_PIPE_ADDR);
    radio.startListening();
  }
  void update() {
    const float duty = 1.0f;
    const unsigned long timeout = 5000ul;
    const unsigned long deepsleepTimeout = 120000ul;
    const float batteryCuttoffVoltage = 6.0f;

    const unsigned long payloadTime = millis();
    const unsigned long payloadTimeDelta = payloadTime - lastPayloadTime;

    voltageAverage = voltageAverage * (1.0 - VOLTAGE_AVERAGE_WEIGHT) + battery.voltage() * VOLTAGE_AVERAGE_WEIGHT;

    // Serial.println("Battery: " + String(voltageAverage));

    if (radio.available()) {
      lastPayload = payload;
      radio.read(&payload, sizeof(payload));

      // Serial.println(String(payload.axis(Axes::AxisX), 3) + ", " + String(payload.axis(Axes::AxisY), 3));

      float drive = 0.0f;
      float steer = 0.0f;
      float strafe = 0.0f;

      drive += payload.axis(Axes::AxisY) * duty;

      if (driveMode == DriveMode::Steer) {
        steer += payload.axis(Axes::AxisX);
      } else if (driveMode == DriveMode::Strafe) {
        strafe += payload.axis(Axes::AxisX) * duty;
      }

      if (payload.buttonPressed(Buttons::ButtonA, lastPayload)) {
        driveMode = (DriveMode)(((int)driveMode + 1) % DRIVE_MODE_COUNT);
      }

      if (driveMode == DriveMode::Steer) {
        if (payload.button(Buttons::ButtonD)) {
          strafe += -1.0f;
        }
        if (payload.button(Buttons::ButtonB)) {
          strafe += 1.0f;
        }
      } else if (driveMode == DriveMode::Strafe) {
        if (payload.button(Buttons::ButtonD)) {
          steer += -1.0f;
        }
        if (payload.button(Buttons::ButtonB)) {
          steer += 1.0f;
        }
      }

      if (payload.button(Buttons::ButtonC)) {
        drive += 1.0f;
      }

      setDrive(drive, steer, strafe);

      lastPayloadTime = payloadTime;
      //     } else if (voltageAverage < batteryCuttoffVoltage || payloadTimeDelta > deepsleepTimeout) {
      // #if defined(ESP32)
      //       esp_deep_sleep_start();
      // #elif defined(ESP8266)
      //       ESP.deepSleep(0);
      // #endif
    } else if (payloadTimeDelta > timeout) {
      // Stop if radio lost
      setDrive();
    }
  }
};
Automaton automaton;

void setup() {
  Serial.begin(115200);

  automaton.init();
}

void loop() {
  automaton.update();

  yield();
}