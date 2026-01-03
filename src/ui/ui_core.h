#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "../dsp/miniacid_engine.h"
#include "display.h"

enum KeyScanCode {
  MINIACID_NO_SCANCODE = 0,
  MINIACID_DOWN,
  MINIACID_UP,
  MINIACID_LEFT,
  MINIACID_RIGHT,
  MINIACID_ESCAPE,
};

enum EventType {
  MINIACID_NO_TYPE = 0,
  MINIACID_KEY_DOWN,
};

struct UIEvent {
  EventType event_type = MINIACID_NO_TYPE;
  KeyScanCode scancode = MINIACID_NO_SCANCODE;
  char key = 0;
  bool alt = false;
};

class IPage {
 public:
  virtual ~IPage() = default;
  virtual void draw(IGfx& gfx, int x, int y, int w, int h) = 0;
  virtual void drawHelpBody(IGfx& gfx, int x, int y, int w, int h) = 0;
  virtual const std::string & getTitle() const = 0;
  virtual bool handleEvent(UIEvent& ui_event) = 0;
  virtual bool handleHelpEvent(UIEvent& ui_event) = 0;
  virtual bool hasHelpDialog() = 0;
};

using AudioGuard = std::function<void(const std::function<void()>&)>;
