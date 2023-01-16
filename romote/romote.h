#include <stdint.h>
#ifndef ROMOTE_H
#define ROMOTE_H

enum Buttons {
  ButtonA,
  ButtonB,
  ButtonC,
  ButtonD,
  ButtonE,
  ButtonF,
  ButtonK
};
enum ButtonFlags {
  ButtonFlagA = 0b00000001u,
  ButtonFlagB = 0b00000010u,
  ButtonFlagC = 0b00000100u,
  ButtonFlagD = 0b00001000u,
  ButtonFlagE = 0b00010000u,
  ButtonFlagF = 0b00100000u,
  ButtonFlagK = 0b01000000u
};
const uint8_t BUTTON_COUNT = 7u;
const uint8_t buttons[] = {
  ButtonFlags::ButtonFlagA,
  ButtonFlags::ButtonFlagB,
  ButtonFlags::ButtonFlagC,
  ButtonFlags::ButtonFlagD,
  ButtonFlags::ButtonFlagE,
  ButtonFlags::ButtonFlagF,
  ButtonFlags::ButtonFlagK
};

enum Axes {
  AxisX,
  AxisY
};
struct StickAxis {
  int inMin, inMax;
  float outMin, outMax;
};
const uint8_t STICK_AXIS_COUNT = 2u;
const StickAxis stickAxes[] = {
  { 0, 1023, -1.0f, 1.0f },
  { 0, 1023, -1.0f, 1.0f },
};
const int STICK_AXIS_MIN = 0;
const int STICK_AXIS_MAX = UINT8_MAX;

struct RadioPayload {
  uint8_t axes[STICK_AXIS_COUNT] = { 0u };
  uint8_t buttonFlags = 0u;

  void update(const uint8_t stickAxisPins[], const uint8_t buttonPins[]) {
    for (int i = 0; i < STICK_AXIS_COUNT; ++i) {
      axes[i] = (uint8_t)constrain(map(analogRead(stickAxisPins[i]), stickAxes[i].inMin, stickAxes[i].inMax, STICK_AXIS_MIN, STICK_AXIS_MAX), STICK_AXIS_MIN, STICK_AXIS_MAX);
    }

    buttonFlags = 0u;
    for (int i = 0; i < BUTTON_COUNT; ++i) {
      if (!digitalRead(buttonPins[i])) buttonFlags |= buttons[i];
    }
  }

  bool changed(const RadioPayload &lastPayload) const {
    bool flag = false;
    for (int i = 0; i < STICK_AXIS_COUNT; ++i) {
      flag = axisChanged(i, lastPayload) || flag;
    }
    for (int i = 0; i < BUTTON_COUNT; ++i) {
      flag = buttonChanged((Buttons)i, lastPayload) || flag;
    }
    return flag;
  }

  float axis(const Axes axis) const {
    const int packedRange = STICK_AXIS_MAX - STICK_AXIS_MIN;
    const float pos = (float)(axes[axis] - STICK_AXIS_MIN) / (float)packedRange;
    const float outRange = stickAxes[axis].outMax - stickAxes[axis].outMin;
    return pos * outRange + stickAxes[axis].outMin;
  }
  bool axisChanged(const uint8_t axis, const RadioPayload &lastPayload) const {
    static const uint8_t threshold = 4;
    return abs((int)this->axes[axis] - (int)lastPayload.axes[axis]) >= threshold;
  }

  bool button(const Buttons button) const {
    return buttonFlags & (0b1u << button);
  }
  bool buttonChanged(const Buttons button, const RadioPayload &lastPayload) const {
    return this->button(button) != lastPayload.button(button);
  }
  bool buttonPressed(const Buttons button, const RadioPayload &lastPayload) const {
    return this->button(button) && !lastPayload.button(button);
  }
  bool buttonReleased(const Buttons button, const RadioPayload &lastPayload) const {
    return !this->button(button) && lastPayload.button(button);
  }
};

#endif