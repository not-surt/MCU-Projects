//Arduino Joystick shield Code
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#include "romote.h"

const uint8_t RADIO_CE = 9u, RADIO_SS = 10u;
RF24 radio(RADIO_CE, RADIO_SS);
const uint8_t RADIO_CHANNEL = 0u;
const uint8_t RADIO_PIPE_ADDR[7] = "RoMote";
RadioPayload payload;

const uint8_t stickAxisPins[] = { A0, A1 };
const uint8_t buttonPins[] = { 2u, 3u, 4u, 5u, 7u, 6u, 8u };

void setup() {
  Serial.begin(9600);

  if (!radio.begin()) {
    Serial.println(F("Radio hardware not responding!"));
    while (true) {}  // hold program in infinite loop to prevent subsequent errors
  } else {
    Serial.println(F("Radio hardware responding normally."));
  }
  radio.setPayloadSize(sizeof(RadioPayload));
  radio.setChannel(RADIO_CHANNEL);
  radio.openWritingPipe(RADIO_PIPE_ADDR);
}

void loop() {
  payload.update(stickAxisPins, buttonPins);
  // char buffer[sizeof(payload) + 1];
  // buffer[sizeof(payload)] = '\0';
  // memcpy(buffer, &payload, sizeof(payload));
  // Serial.println(buffer);
  radio.write(&payload, sizeof(payload));
}