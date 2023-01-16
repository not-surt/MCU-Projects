#include <TimeLib.h>
#include <Adafruit_NeoPixel.h>

#define TIME_HEADER  "T"   // Header tag for serial time sync message
#define TIME_REQUEST  7    // ASCII bell character requests a time sync message 
#define TIME_SYNC_LED_GPIO 25

#define NEOPIXEL_PIN 6
#define NEOPIXEL_COUNT 60
#define NEOPIXEL_OFFSET 30
#define NEOPIXEL_BRIGHTNESS 25

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

int mod(const int dividend, const int divisor) {
  int remainder = dividend % divisor;
  return remainder < 0 ? remainder + divisor : remainder;
}

int wrap(const int value, const int min_, const int max_)
{
    return min_ + mod(value - min_, max_ - min_);
}

int clamp(const int value, const int min_, const int max_)
{
  return max(min(value, max_), min_);
}

void drawHands(Adafruit_NeoPixel &strip, const int hour, const int minute, const int second, const int stripOffset = 0)
{
  const float pixelsPerTick = strip.numPixels() / 60.0;
  const float pixelsPerHour =  pixelsPerTick * 5;
  for (int i = 0; i < strip.numPixels(); ++i) {
    int pixel = wrap(i + stripOffset, 0, strip.numPixels());
    float pixelHour = pixel / pixelsPerHour;
    float pixelTick = pixel / pixelsPerTick;
    int colour[3] = {0, 0, 0};
    if ((int)floor(pixelHour) == hour) {
      colour[0] = 255;
    }
    if ((int)floor(pixelTick) == minute) {
      colour[1] = 255;
    }
    if ((int)floor(pixelTick) == second) {
      colour[2] = 255;
    }
    if ((int)floor(pixelTick) % 5 == 0) {
      colour[0] += 16;
      colour[1] += 16;
      colour[2] += 16;
    }    
    // strip.setPixelColor(pixel, strip.Color(255, 255, (pixel % 5) * 12));
    strip.setPixelColor(i, strip.Color(clamp(colour[0], 0, 255), clamp(colour[1], 0, 255), clamp(colour[2], 0, 255)));
    // Serial.println("POOP!" + String(i) + " of " + String(strip.numPixels()));
  }
  strip.show();
}

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

void setup() {
  Serial.begin(9600);
  setSyncProvider(requestSync);  //set function to call when sync required

  strip.begin();
  strip.setBrightness(NEOPIXEL_BRIGHTNESS);
  strip.show(); // Initialize all pixels to 'off'
}

void loop() {
  if (Serial.available()) {
    processSyncMessage();
  }

  Serial.println(String(hour()) + String(":") + String(minute()) + String(":") + String(second()));
  drawHands(strip, hour(), minute(), second(), NEOPIXEL_OFFSET);
}

void processSyncMessage() {
  unsigned long pctime;
  const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013

  if(Serial.find(TIME_HEADER)) {
     pctime = Serial.parseInt();
     if( pctime >= DEFAULT_TIME) { // check the integer is a valid time (greater than Jan 1 2013)
       setTime(pctime); // Sync Arduino clock to the time received on the serial port
     }
  }
}

time_t requestSync()
{
  Serial.write(TIME_REQUEST);  
  return 0; // the time will be sent later in response to serial mesg
}
