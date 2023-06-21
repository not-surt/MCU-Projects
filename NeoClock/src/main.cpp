#include <cmath>
#include <vector>

#include <TimeLib.h>
#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <RTClib.h>

int mod(const int dividend, const int divisor) {
  const int remainder = dividend % divisor;
  return remainder < 0 ? remainder + divisor : remainder;
}

int wrap(const int value, const int min_, const int max_) {
    return min_ + mod(value - min_, max_ - min_);
}

int clamp(const int value, const int min_, const int max_) {
  return max(min(value, max_), min_);
}

uint8_t colourRed(const uint32_t colour) {
  return (uint8_t)((colour >> 16) & 255);
}
uint8_t colourGreen(const uint32_t colour) {
  return (uint8_t)((colour >> 8) & 255);
}
uint8_t colourBlue(const uint32_t colour) {
  return (uint8_t)((colour >> 0) & 255);
}
uint32_t colourAdd(const uint32_t a, const uint32_t b) {
  return Adafruit_NeoPixel::Color(
    (uint8_t)clamp((int16_t)colourRed(a) + (int16_t)colourRed(b), 0, 255),
    (uint8_t)clamp((int16_t)colourGreen(a) + (int16_t)colourGreen(b), 0, 255),
    (uint8_t)clamp((int16_t)colourBlue(a) + (int16_t)colourBlue(b), 0, 255)
  );
}
uint32_t colourSubtract(const uint32_t a, const uint32_t b) {
  return Adafruit_NeoPixel::Color(
    (uint8_t)clamp((int16_t)colourRed(a) - (int16_t)colourRed(b), 0, 255),
    (uint8_t)clamp((int16_t)colourGreen(a) - (int16_t)colourGreen(b), 0, 255),
    (uint8_t)clamp((int16_t)colourBlue(a) - (int16_t)colourBlue(b), 0, 255)
  );
}
uint32_t colourMultiply(const uint32_t a, const uint32_t b) {
  return Adafruit_NeoPixel::Color(
    (uint8_t)clamp((int16_t)((uint16_t)colourRed(a) * (uint16_t)colourRed(b) / 255), 0, 255),
    (uint8_t)clamp((int16_t)((uint16_t)colourGreen(a) * (uint16_t)colourGreen(b) / 255), 0, 255),
    (uint8_t)clamp((int16_t)((uint16_t)colourBlue(a) * (uint16_t)colourBlue(b) / 255), 0, 255)
  );
}

void drawHands(Adafruit_NeoPixel &strip, const int hour, const int minute, const int second, const float stripOffset = 0.0f) {
  static const bool toggleDivisions = true;

  static const uint32_t originColour = strip.Color(255, 127, 127);
  static const uint32_t divisionColour = strip.Color(31, 31, 31);
  static const uint32_t divisionColourAlt = strip.Color(0, 0, 0);
  static const uint32_t hourColour = strip.Color(255, 0, 0);
  static const uint32_t minuteColour = strip.Color(0, 255, 0);
  static const uint32_t secondColour = strip.Color(0, 0, 255);

  static const uint8_t divisionCount = 12;
  static const uint8_t hourCount = 12;
  static const uint8_t minuteCount = 60;
  static const uint8_t secondCount = 60;

  const float divisionPixels = (float)strip.numPixels() / (float)divisionCount;
  const float hourPixels = (float)strip.numPixels() / (float)hourCount;
  const float minutePixels = (float)strip.numPixels() / (float)minuteCount;
  const float secondPixels = (float)strip.numPixels() / (float)secondCount;

  auto bandOverlap = [&strip](const float bandACentre, const float bandAWidth, const float bandBCentre, const float bandBWidth) -> float {
    const float offset = abs(bandACentre - bandBCentre);
    const float spacing = (bandAWidth + bandBWidth) / 2.0;
    return  offset >= spacing ? 0.0f : min(spacing - offset, min(bandAWidth, bandBWidth));
    // const float centreDistance = min(offset, strip.numPixels() - offset);
    // return min(min(bandAWidth, bandBWidth), max((bandAWidth / 2.0f + bandBWidth / 2.0f) - centreDistance, 0.0f));
    // const float halfBandAWidth = bandAWidth / 2.0f;
    // const float halfBandBWidth = bandBWidth / 2.0f;
    // const float overlapStart = max(bandACentre - halfBandAWidth, bandBCentre - halfBandBWidth);
    // const float overlapEnd = min(bandACentre + halfBandAWidth, bandBCentre + halfBandBWidth);
    // return overlapEnd - overlapStart;
  };

  // auto bandOverlapWrapped = [&strip](const float bandACentre, const float bandAWidth, const float bandBCentre, const float bandBWidth) -> float {
  //   const float halfBandAWidth = bandAWidth / 2.0f;
  //   const float halfBandBWidth = bandBWidth / 2.0f;
  //   const int wrapMin = mod(min(bandACentre - halfBandAWidth, bandBCentre - halfBandBWidth), strip.numPixels());
  //   const int wrapMax = mod(max(bandACentre + halfBandAWidth, bandBCentre + halfBandBWidth), strip.numPixels());
  //   for (int i = wrapMin; i <= wrapMax; ++i) {
  //     const float bandACentreWrapped = 0.0;
  //   }
  //   return 0.0f;
  // };

  // Serial.println(bandOverlap(0.25f, 1.0f, 1.0f, 2.0f));
  // Serial.println(bandOverlap(0.75f, 1.0f, 1.0f, 2.0f));

  struct Band {
    float pos;
    float width;
    uint32_t colour;
  };
  std::vector<Band> bands{
    {0.5f, 2, 0x00ffffff},
    // {nearestDivision * divisionPixels + 0.5f, 2, 0x007f7f7f},
    {hour * hourPixels, 4, 0x00ff0000},
    {minute * minutePixels, 2, 0x0000ff00},
    {second * secondPixels, 1, 0x000000ff},
  };

  for (int i = 0; i < strip.numPixels(); ++i) {
    const uint16_t pixel = wrap(i + stripOffset, 0, strip.numPixels() - 1);
    const float pixelCentre = pixel + 0.5f;

    const uint8_t nearestDivision = (uint8_t)round(pixelCentre / divisionPixels);
    const uint8_t nearestHour = (uint8_t)round(pixelCentre / hourPixels);
    const uint8_t nearestMinute = (uint8_t)round(pixelCentre / minutePixels);
    const uint8_t nearestSecond = (uint8_t)round(pixelCentre / secondPixels);
    
    const float pixelHour = round((pixel + 0.5f) / hourPixels);

    uint32_t colour = 0;

    for (auto &band : bands) {
      const float overlap = bandOverlap(pixelCentre, 1.0f, band.pos + 0.5f, band.width);
      // Serial.println("Overlap: " + String(overlap));
      if (overlap > 0.5f) {
        colour = colourAdd(colour, band.colour);
      }
    }

    strip.setPixelColor(i, colour);
  }

  strip.show();
}

const char *const TIME_HEADER = "T";   // Header tag for serial time sync message
const char TIME_REQUEST = 7;    // ASCII bell character requests a time sync message 

void processSyncMessage() {
  unsigned long pctime;
  const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013

  if(Serial.find(TIME_HEADER)) {
     pctime = Serial.parseInt();
     if(pctime >= DEFAULT_TIME) { // check the integer is a valid time (greater than Jan 1 2013)
       setTime(pctime); // Sync Arduino clock to the time received on the serial port
     }
  }
}

time_t requestSync() {
  Serial.write(TIME_REQUEST);  
  return 0; // the time will be sent later in response to serial mesg
}

Adafruit_NeoPixel *strip = nullptr;

RTC_DS1307 rtc;

void setup() {
  Serial.begin(115200);
  Serial.println("SETUP!");

  setSyncProvider(requestSync);  //set function to call when sync required

  setTime(7, 45, 11, 5, 1, 2023);

  const uint8_t neopixelPin = 2;
  const uint16_t neopixelCount = 60;

  strip = new Adafruit_NeoPixel(neopixelCount, neopixelPin, NEO_GRB + NEO_KHZ800);

  strip->begin();
  strip->setBrightness(31);
  strip->show(); // Initialize all pixels to 'off'

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while(1);
  }
   rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

void loop() {
  // Serial.println("LOOP!");
  DateTime now = rtc.now();
  // Serial.print(now.year(), DEC);
  // Serial.print('/');
  // Serial.print(now.month(), DEC);
  // Serial.print('/');
  // Serial.print(now.day(), DEC);
  // Serial.print(' ');
  // Serial.print(now.hour(), DEC);
  // Serial.print(':');
  // Serial.print(now.minute(), DEC);
  // Serial.print(':');
  // Serial.print(now.second(), DEC);
  // Serial.println();

  if (Serial.available()) {
    processSyncMessage();
  }

  // Serial.println(String(hour()) + String(":") + String(minute()) + String(":") + String(second()));
  drawHands(*strip, hour(), minute(), second(), 0);
}
