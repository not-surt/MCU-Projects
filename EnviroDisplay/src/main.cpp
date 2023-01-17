#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BME680.h>
#include <esp_timer.h>

TwoWire i2c0 = TwoWire(0);
TwoWire i2c1 = TwoWire(1);

LiquidCrystal_PCF8574 lcd(0x3f); 

// Adafruit_BMP280 bmp(&i2c1);
// Adafruit_AHTX0 aht;
Adafruit_BME680 bme(&i2c1);

// hw_timer_t *sensorsTimer = nullptr;
// hw_timer_t *displayTimer = nullptr;	
// hw_timer_t *sleepTimer = nullptr;	
// hw_timer_t *stayAwakeTimer = nullptr;	
esp_timer_handle_t sensorsTimer;
esp_timer_handle_t displayTimer;
esp_timer_handle_t sleepTimer;
esp_timer_handle_t stayAwakeTimer;

portMUX_TYPE doplerDetectMux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE sensorsTimerMux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE displayTimerMux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE sleepTimerMux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE stayAwakeTimerMux = portMUX_INITIALIZER_UNLOCKED;

const int doplerPin = 15;
volatile bool wakeUp = true;
void IRAM_ATTR doplerDetectISR() {
  portENTER_CRITICAL_ISR(&doplerDetectMux);
  wakeUp = true;
  portEXIT_CRITICAL_ISR(&doplerDetectMux);
}

bool sensorsLogging = true;
bool sensorsChanged = true;
struct Sensors {
  float temperature;
  float humidity;
  float pressure;
  float gas;
} sensors{};
volatile bool sensorsNeedUpdate = true;
void IRAM_ATTR sensorsUpdateISR(void *arg) {
  portENTER_CRITICAL_ISR(&sensorsTimerMux);
  sensorsNeedUpdate = true;
  portEXIT_CRITICAL_ISR(&sensorsTimerMux);
}

volatile bool displayNeedsUpdate = true;
void IRAM_ATTR displayUpdateISR(void *arg) {
  portENTER_CRITICAL_ISR(&displayTimerMux);
  displayNeedsUpdate = true;
  portEXIT_CRITICAL_ISR(&displayTimerMux);
}

volatile bool goToSleep = false;
void IRAM_ATTR sleepISR(void *arg) {
  portENTER_CRITICAL_ISR(&sleepTimerMux);
  goToSleep = true;
  portEXIT_CRITICAL_ISR(&sleepTimerMux);
}

volatile bool stayAwake = false;
void IRAM_ATTR wakeISR(void *arg) {
  portENTER_CRITICAL_ISR(&stayAwakeTimerMux);
  stayAwake = digitalRead(doplerPin);
  portEXIT_CRITICAL_ISR(&stayAwakeTimerMux);
}

void setup()
{
  Serial.begin(115200);

  i2c0.begin(21, 22);
  i2c0.setClock(100000);
  lcd.begin(20, 4, i2c0);

  i2c1.begin(18, 5); 
  i2c1.setClock(100000);
  // bmp.begin(0x77);
  // bmp.setSampling(Adafruit_BMP280::MODE_NORMAL, Adafruit_BMP280::SAMPLING_X2, Adafruit_BMP280::SAMPLING_X16, Adafruit_BMP280::FILTER_X16, Adafruit_BMP280::STANDBY_MS_500);
  // aht.begin(&i2c1, 0, 0x38);

  bme.begin(0x77);
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150);

  pinMode(doplerPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(doplerPin), doplerDetectISR, RISING);

  // sensorsTimer = timerBegin(0, 80, true);
  // timerAttachInterrupt(sensorsTimer, &sensorsUpdateISR, true);
  // timerAlarmWrite(sensorsTimer, 1000000 * 5, true);
  // timerAlarmEnable(sensorsTimer);

  // displayTimer = timerBegin(1, 80, true);
  // timerAttachInterrupt(displayTimer, &displayUpdateISR, true);
  // timerAlarmWrite(displayTimer, 1000000 * 1, true);
  // timerAlarmEnable(displayTimer);

  // sleepTimer = timerBegin(2, 80, true);
  // timerAttachInterrupt(sleepTimer, &sleepISR, true);
  // timerAlarmWrite(sleepTimer, 1000000 * 20, true);

  // stayAwakeTimer = timerBegin(3, 80, true);
  // timerAttachInterrupt(stayAwakeTimer, &wakeISR, true);
  // timerAlarmWrite(stayAwakeTimer, 1000000, true);
  // timerAlarmEnable(stayAwakeTimer);

  esp_sleep_enable_ext0_wakeup((gpio_num_t)doplerPin, 1);

  // esp_timer_start_once(timer, 1000000);
  esp_timer_create_args_t sensorsTimerArgs{&sensorsUpdateISR, nullptr, ESP_TIMER_TASK, "name", true};
  esp_timer_create(&sensorsTimerArgs, &sensorsTimer);
  esp_timer_start_periodic(sensorsTimer, 1000000);

  esp_timer_create_args_t displayTimerArgs{&displayUpdateISR, nullptr, ESP_TIMER_TASK, "name", true};
  esp_timer_create(&displayTimerArgs, &displayTimer);
  esp_timer_start_periodic(displayTimer, 1000000);

  esp_timer_create_args_t sleepTimerArgs{&sleepISR, nullptr, ESP_TIMER_TASK, "name", true};
  esp_timer_create(&sleepTimerArgs, &sleepTimer);
  esp_timer_start_periodic(sleepTimer, 1000000 * 20);

  esp_timer_create_args_t stayAwakeTimerArgs{&wakeISR, nullptr, ESP_TIMER_TASK, "name", true};
  esp_timer_create(&stayAwakeTimerArgs, &stayAwakeTimer);
  esp_timer_start_periodic(stayAwakeTimer, 1000000);
}

void loop()
{

  // Update sensors
  if (sensorsNeedUpdate) {
    portENTER_CRITICAL(&sensorsTimerMux);
    sensorsNeedUpdate = false;
    portEXIT_CRITICAL(&sensorsTimerMux);

    // sensors_event_t humidity, temperature;
    // aht.getEvent(&humidity, &temperature);
    // sensors.temperature = (temperature.temperature + bmp.readTemperature()) / 2.0f;
    // sensors.humidity = humidity.relative_humidity;
    // sensors.pressure = bmp.readPressure();

    unsigned long endTime = bme.beginReading();
    if (!bme.endReading()) {
      sensorsNeedUpdate = true;
      return;
    }

    sensors.temperature = bme.temperature;
    sensors.humidity = bme.humidity;
    sensors.pressure = bme.pressure;
    sensors.gas = bme.gas_resistance;

    sensorsChanged = true;
  }

  // Update display
  if (sensorsChanged && displayNeedsUpdate) {
    portENTER_CRITICAL(&displayTimerMux);
    displayNeedsUpdate = false;
    portEXIT_CRITICAL(&displayTimerMux);

    sensorsChanged = false;

    char num_str_buf[16];

    const uint8_t labelColumn = 0;
    const uint8_t valueColumn = 12;
    const uint8_t valueWidth = 5;
    const uint8_t valuePrecision = 1;
    
    lcd.home();
    lcd.clear();

    lcd.setCursor(labelColumn, 0);
    lcd.print(F("Temperature:"));
    lcd.setCursor(valueColumn, 0);
    lcd.print(dtostrf(sensors.temperature, valueWidth, valuePrecision, num_str_buf));
    lcd.print((char)0xdf);
    lcd.print(F("C"));

    lcd.setCursor(labelColumn, 1);
    lcd.print(F("Humidity:"));
    lcd.setCursor(valueColumn, 1);
    lcd.print(dtostrf(sensors.humidity, valueWidth, valuePrecision, num_str_buf));
    lcd.print(F("%RH"));
    
    lcd.setCursor(labelColumn, 2);
    lcd.print(F("Pressure:"));
    lcd.setCursor(valueColumn, 2);
    lcd.print(dtostrf(sensors.pressure / 1000.f, valueWidth, valuePrecision, num_str_buf));
    lcd.print(F("kPa"));
    
    lcd.setCursor(labelColumn, 3);
    lcd.print(F("Gas:"));
    lcd.setCursor(valueColumn, 3);
    lcd.print(dtostrf(sensors.gas / 1000.f, valueWidth, valuePrecision, num_str_buf));
    lcd.print(F("kOh"));
  }
  
  // Sleep
  if (goToSleep) {
    portENTER_CRITICAL(&sleepTimerMux);
    goToSleep = false;
    portEXIT_CRITICAL(&sleepTimerMux);

    Serial.println("go to sleep");
    // timerAlarmDisable(sleepTimer);
    esp_timer_stop(sleepTimer);

    lcd.setBacklight(false);

    // esp_deep_sleep_start();
    esp_light_sleep_start();
  }

  // Wake
  if (wakeUp || stayAwake) {
    if (wakeUp) Serial.println("wake up");
    if (stayAwake) Serial.println("stay awake");
    portENTER_CRITICAL(&doplerDetectMux);
    wakeUp = false;
    portEXIT_CRITICAL(&doplerDetectMux);
    portENTER_CRITICAL(&stayAwakeTimerMux);
    stayAwake = false;
    portEXIT_CRITICAL(&stayAwakeTimerMux);

    // timerRestart(sleepTimer);
    // timerAlarmEnable(sleepTimer);
    esp_timer_stop(sleepTimer);
    esp_timer_start_periodic(sleepTimer, 1000000 * 20);

    lcd.setBacklight(true);
  }
}
