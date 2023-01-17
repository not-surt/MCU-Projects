#include <cmath>

#include <TimeLib.h>
#include <Adafruit_NeoPixel.h>

int mod(const int dividend, const int divisor) {
  const int remainder = dividend % divisor;
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

void drawHands(Adafruit_NeoPixel &strip, const int hour, const int minute, const int second, const float stripOffset = 0.0f)
{
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

  const uint8_t divisionPixels = strip.numPixels() / divisionCount;
  const uint8_t hourPixels = strip.numPixels() / hourCount;
  const uint8_t minutePixels = strip.numPixels() / minuteCount;
  const uint8_t secondPixels = strip.numPixels() / secondCount;

  static const uint8_t originMarker = 2;
  static const uint8_t divisionMarker = 2;
  static const uint8_t hourMarker = 6;
  static const uint8_t minuteMarker = 4;
  static const uint8_t secondMarker = 2;

  auto inBand = [&strip](const float pos, const float bandCentre, const float bandWidth) -> bool {
    const int testPos = pos + (abs(bandCentre - pos) <= (strip.numPixels() / 2) ? 0 : bandCentre > pos ? strip.numPixels() : -strip.numPixels());
    const int halfBandWidth = bandWidth / 2;
    return (testPos >= bandCentre - halfBandWidth) && (testPos < bandCentre + halfBandWidth);
  };

  auto bandOverlap = [&strip](const float bandACentre, const float bandAWidth, const float bandBCentre, const float bandBWidth) -> float {
    // const float offset = abs(bandACentre - bandBCentre);
    // const float centreDistance = min(offset, strip.numPixels() - offset);
    // return min(min(bandAWidth, bandBWidth), max((bandAWidth / 2.0f + bandBWidth / 2.0f) - centreDistance, 0.0f));
    const float halfBandAWidth = bandAWidth / 2.0f;
    const float halfBandBWidth = bandBWidth / 2.0f;
    const float overlapStart = max(bandACentre - halfBandAWidth, bandBCentre - halfBandBWidth);
    const float overlapEnd = min(bandACentre + halfBandAWidth, bandBCentre + halfBandBWidth);
    return overlapEnd - overlapStart;
  };

  auto bandOverlapWrapped = [&strip](const float bandACentre, const float bandAWidth, const float bandBCentre, const float bandBWidth) -> float {
    const float halfBandAWidth = bandAWidth / 2.0f;
    const float halfBandBWidth = bandBWidth / 2.0f;
    const int wrapMin = mod(min(bandACentre - halfBandAWidth, bandBCentre - halfBandBWidth), strip.numPixels());
    const int wrapMax = mod(max(bandACentre + halfBandAWidth, bandBCentre + halfBandBWidth), strip.numPixels());
    for (int i = wrapMin; i <= wrapMax; ++i) {
      const float bandACentreWrapped = 0.0;
    }
    return 0.0f;
  };

  // Serial.println(bandOverlap(0.25f, 1.0f, 1.0f, 2.0f));
  // Serial.println(bandOverlap(0.75f, 1.0f, 1.0f, 2.0f));

  for (int i = 0; i < strip.numPixels(); ++i) {
    const uint16_t pixel = wrap(i + stripOffset, 0, strip.numPixels() - 1);
    const float pixelCentre = pixel + 0.5f;

    const uint8_t nearestDivision = (uint8_t)round(pixelCentre / divisionPixels);
    const uint8_t nearestHour = (uint8_t)round(pixelCentre / hourPixels);
    const uint8_t nearestMinute = (uint8_t)round(pixelCentre / minutePixels);
    const uint8_t nearestSecond = (uint8_t)round(pixelCentre / secondPixels);
    
    const float pixelHour = round((pixel + 0.5f) / hourPixels);

    uint32_t colour = 0;
    // Origin
    // if (inBand(pixel, 0, originMarker)) {
    if (bandOverlap(pixelCentre, 1.0f, 0.5f, originMarker) > 0.5f) {
      colour = colourAdd(colour, originColour);
    }    
    // Divisions
    // if (inBand(pixel, nearestDivision * divisionPixels, divisionMarker)) {
    if (bandOverlap(pixelCentre, 1.0f, nearestDivision * divisionPixels + 0.5f, divisionMarker) > 0.5f) {
      colour = colourAdd(colour, (!toggleDivisions || (second % 2 == 0)) ? divisionColour : divisionColourAlt);
    }    
    // // Hour
    // if (inBand(pixel, hour * hourPixels, hourMarker)) {
    if (bandOverlap(pixelCentre, 1.0f, hour * hourPixels + 0.5f, hourMarker) > 0.5f) {
      colour = colourAdd(colour, hourColour);
    }
    // // Minute
    // if (inBand(pixel, minute * minutePixels, minuteMarker)) {
    if (bandOverlap(pixelCentre, 1.0f, minute * minutePixels + 0.5f, minuteMarker) > 0.5f) {
      colour = colourAdd(colour, minuteColour);
    }
    // // Second
    // if (inBand(pixel, second * secondPixels, secondMarker)) {
    if (bandOverlap(pixelCentre, 1.0f, second * secondPixels + 0.5f, secondMarker) > 0.5f) {
      colour = colourAdd(colour, secondColour);
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

time_t requestSync()
{
  Serial.write(TIME_REQUEST);  
  return 0; // the time will be sent later in response to serial mesg
}

Adafruit_NeoPixel *strip = nullptr;

void setup() {
  Serial.begin(115200);
  setSyncProvider(requestSync);  //set function to call when sync required

  setTime(7, 45, 11, 5, 1, 2023);

  const uint8_t neopixelPin = 2;
  const uint16_t neopixelCount = 60;

  strip = new Adafruit_NeoPixel(neopixelCount, neopixelPin, NEO_GRB + NEO_KHZ800);

  strip->begin();
  strip->setBrightness(31);
  strip->show(); // Initialize all pixels to 'off'
}

void loop() {
  if (Serial.available()) {
    processSyncMessage();
  }

  Serial.println(String(hour()) + String(":") + String(minute()) + String(":") + String(second()));
  drawHands(*strip, hour(), minute(), second(), 0);
}
