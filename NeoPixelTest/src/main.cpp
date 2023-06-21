#include <Adafruit_NeoPixel.h>
#include <vector>
#include <limits>
#include <array>

typedef std::array<uint8_t, 3> Colour;
const std::vector<Colour> colours = {
  {255, 0, 0},
  {0, 255, 0},
  {0, 0, 255},
  // {255, 255, 255},
  // {255, 255, 255},
  // {255, 255, 255},
  // {255, 255, 255},
  // {255, 255, 255},
  // {0, 0, 0},
  // {0, 0, 0},
  // {255, 255, 255},
  // {0, 0, 0},
  // {0, 0, 0},
  // {255, 255, 255},
  // {0, 0, 0},
  // {0, 0, 0},
  // {255, 0, 0},
  // {255, 255, 0},
  // {0, 255, 0},
  // {0, 255, 255},
  // {0, 0, 255},
  // {255, 0, 255},
};

const uint32_t PERIOD = colours.size() * 1000;

uint32_t lastTime;
uint32_t periodTime;

const uint8_t PIN = 19;
const uint8_t NUMPIXELS = 7;
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  pixels.begin();
  lastTime = millis();
  periodTime = 0;
}

void loop() {
  const uint32_t thisTime = millis();
  const uint32_t deltaTime = thisTime > lastTime ? thisTime - lastTime : thisTime + (std::numeric_limits<decltype(thisTime)>::max() - lastTime);
  periodTime = (periodTime + deltaTime) % PERIOD;
  const uint16_t segmentCount = colours.size();
  const uint16_t segment = periodTime * segmentCount / PERIOD;
  const uint32_t segmentStart = PERIOD * segment / segmentCount;
  const uint32_t segmentEnd = PERIOD * (segment + 1) / segmentCount;
  const uint32_t segmentLength = segmentEnd - segmentStart;
  const uint32_t segmentTime = periodTime - segmentStart;
  const uint16_t indexA = segment;
  const uint16_t indexB = (segment + 1) % colours.size();
  const Colour &colourA = colours[indexA];
  const Colour &colourB = colours[indexB];
  Colour colour;
  auto lerp = [](const float v0, const float v1, const float t) {
    return v0 + t * (v1 - v0);
  };
  const float t = (float)segmentTime / (float)segmentLength;
  for (int i = 0; i <= std::tuple_size<Colour>(); i++) {
    colour[i] = lerp(colourA[i], colourB[i], t);
    // colour[i] = (uint8_t)((int32_t)colourA[i] + (int32_t)segmentTime * ((int32_t)colourB[i] - (int32_t)colourA[i]) / (int32_t)segmentLength);
  }
  for (int i = 0; i < NUMPIXELS; i++) { // For each pixel...
    pixels.setPixelColor(i, pixels.Color(colour[0], colour[1], colour[2]));
  }
  pixels.show();   // Send the updated pixel colors to the hardware.
  lastTime = thisTime;
  Serial.println(String(periodTime) + ": " + String(segment) + "(" + String(indexA) + "," + String(indexB) + ")" + " - " + "RGB(" + String(colour[0]) + ", " + String(colour[1]) + ", " + String(colour[2]) + ")");

  // delay(100);
}
