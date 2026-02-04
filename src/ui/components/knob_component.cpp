#include "knob_component.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <utility>

#include "../../miniacid_config.h"
#include "../ui_colors.h"
#include "../ui_utils.h"

namespace {
inline constexpr IGfxColor kFocusColor = IGfxColor(0xB36A00);
constexpr int kKnobLutScale = 1024;

constexpr int kKnobLutSteps =
    (MINIACID_KNOB_LUT_STEPS < 8 ? 8 : MINIACID_KNOB_LUT_STEPS);
int16_t s_knobCos[kKnobLutSteps];
int16_t s_knobSin[kKnobLutSteps];
bool s_knobLutReady = false;

void initKnobLut() {
  if (s_knobLutReady) return;
  constexpr float kDegToRad = 3.14159265f / 180.0f;
  for (int i = 0; i < kKnobLutSteps; ++i) {
    float t = static_cast<float>(i) / static_cast<float>(kKnobLutSteps - 1);
    float deg = 135.0f + t * 270.0f;
    float angle = deg * kDegToRad;
    s_knobCos[i] = static_cast<int16_t>(roundf(cosf(angle) * kKnobLutScale));
    s_knobSin[i] = static_cast<int16_t>(roundf(sinf(angle) * kKnobLutScale));
  }
  s_knobLutReady = true;
}

int automationIndicatorSize(int font_height) {
  int size = 5;
  if (size > font_height - 2) size = font_height - 2;
  if (size < 2) return 0;
  return size;
}

void drawAutomationIndicator(IGfx& gfx, int x, int y, int size, bool enabled) {
  IGfxColor square_color = IGfxColor::Yellow();
  if (enabled) {
    gfx.fillRect(x, y, size, size, square_color);
  } else {
    gfx.drawRect(x, y, size, size, square_color);
  }
}
} // namespace

KnobComponent::KnobComponent(const Parameter& param, IGfxColor ring_color,
                             IGfxColor indicator_color,
                             std::function<void(int)> adjust_fn,
                             KnobAutomationAccess access, int param_id)
    : param_(param),
      ring_color_(ring_color),
      indicator_color_(indicator_color),
      adjust_fn_(std::move(adjust_fn)),
      access_(std::move(access)),
      param_id_(param_id) {}

void KnobComponent::setValue(int direction) {
  if (adjust_fn_) {
    adjust_fn_(direction);
  }
}

bool KnobComponent::handleEvent(UIEvent& ui_event) {
  if (ui_event.event_type == MINIACID_KEY_DOWN && isFocused()) {
    switch (ui_event.scancode) {
      case MINIACID_UP:
        setValue(1);
        return true;
      case MINIACID_DOWN:
        setValue(-1);
        return true;
      default:
        break;
    }
  }
  if (ui_event.event_type == MINIACID_MOUSE_DOWN) {
    if (ui_event.button != MOUSE_BUTTON_LEFT) {
      return false;
    }
    if (!contains(ui_event.x, ui_event.y)) {
      return false;
    }
    dragging_ = true;
    last_drag_y_ = ui_event.y;
    drag_accum_ = 0;
    return true;
  }

  if (ui_event.event_type == MINIACID_MOUSE_UP) {
    if (!dragging_) {
      return false;
    }
    dragging_ = false;
    drag_accum_ = 0;
    return true;
  }

  if (ui_event.event_type == MINIACID_MOUSE_DRAG) {
    if (!dragging_) {
      return false;
    }
    int delta = ui_event.dy;
    if (delta == 0) {
      delta = ui_event.y - last_drag_y_;
    }
    last_drag_y_ = ui_event.y;
    drag_accum_ += delta;
    constexpr int kPixelsPerStep = 4;
    while (drag_accum_ <= -kPixelsPerStep) {
      setValue(1);
      drag_accum_ += kPixelsPerStep;
    }
    while (drag_accum_ >= kPixelsPerStep) {
      setValue(-1);
      drag_accum_ -= kPixelsPerStep;
    }
    return true;
  }

  if (ui_event.event_type == MINIACID_MOUSE_SCROLL) {
    if (!contains(ui_event.x, ui_event.y)) {
      return false;
    }
    if (ui_event.wheel_dy > 0) {
      setValue(1);
      return true;
    }
    if (ui_event.wheel_dy < 0) {
      setValue(-1);
      return true;
    }
  }

  return false;
}

void KnobComponent::draw(IGfx& gfx) {
  const Rect& bounds = getBoundaries();
  if (bounds.w <= 0 || bounds.h <= 0) return;

  int font_h = gfx.fontHeight();
  if (font_h < 1) font_h = 1;
  int text_gap = 2;
  int value_band_h = font_h;
  int label_band_h = font_h;
  int available_h = bounds.h - value_band_h - label_band_h - text_gap * 2;
  if (available_h < 0) available_h = 0;

  int radius = std::min(bounds.w, available_h) / 2;
  int cx = bounds.x + bounds.w / 2;
  int knob_top = bounds.y + value_band_h + text_gap;
  int knob_center_y = knob_top + available_h / 2;

  if (radius > 0) {
#if !MINIACID_PERF_TEST_DISABLE_KNOB_DRAW
    initKnobLut();

    float norm = std::clamp(param_.normalized(), 0.0f, 1.0f);
    int idx = static_cast<int>(norm * static_cast<float>(kKnobLutSteps - 1) + 0.5f);
    if (idx < 0) idx = 0;
    if (idx >= kKnobLutSteps) idx = kKnobLutSteps - 1;

    gfx.drawKnobFace(cx, knob_center_y, radius, ring_color_, COLOR_BLACK);

    int radius_inner = radius - 2;
    if (idx != last_lut_index_ || radius_inner != last_radius_) {
      int scaled_x = s_knobCos[idx] * radius_inner;
      int scaled_y = s_knobSin[idx] * radius_inner;
      last_ix_ = cx + scaled_x / kKnobLutScale;
      last_iy_ = knob_center_y + scaled_y / kKnobLutScale;
      last_lut_index_ = idx;
      last_radius_ = radius_inner;
    }

    drawLineColored(gfx, cx, knob_center_y, last_ix_, last_iy_, indicator_color_);
#endif
  }

  const char* label = param_.label();
  if (!label) {
    label = "";
  }
  gfx.setTextColor(COLOR_LABEL);
  int label_w = textWidth(gfx, label);
  int label_x = cx - label_w / 2;
  if (label_w <= bounds.w) {
    if (label_x < bounds.x) label_x = bounds.x;
    if (label_x + label_w > bounds.x + bounds.w) {
      label_x = bounds.x + bounds.w - label_w;
    }
  } else {
    label_x = bounds.x;
  }
  int label_y = bounds.y + bounds.h - label_band_h;
  if (label_y < bounds.y) label_y = bounds.y;
  gfx.drawText(label_x, label_y, label);
  if (access_.lane && param_id_ >= 0) {
    const AutomationLane* lane = access_.lane(param_id_);
    if (lane && lane->hasNodes()) {
      int size = automationIndicatorSize(gfx.fontHeight());
      if (size > 0) {
        int square_x = label_x + label_w + 2;
        int square_y = label_y + (label_band_h - size) / 2;
        if (square_x + size > bounds.x + bounds.w) {
          square_x = bounds.x + bounds.w - size;
        }
        if (square_x < bounds.x) square_x = bounds.x;
        if (square_y < bounds.y) square_y = bounds.y;
        if (square_y + size > bounds.y + bounds.h) {
          square_y = bounds.y + bounds.h - size;
        }
        if (size > 0) {
          drawAutomationIndicator(gfx, square_x, square_y, size, lane->enabled);
        }
      }
    }
  }

  char buf[48];
  const char* unit = param_.unit();
  float value = param_.value();
  if (unit && unit[0]) {
    snprintf(buf, sizeof(buf), "%.0f %s", value, unit);
  } else {
    snprintf(buf, sizeof(buf), "%.2f", value);
  }
  int val_w = textWidth(gfx, buf);
  int val_x = cx - val_w / 2;
  if (val_w <= bounds.w) {
    if (val_x < bounds.x) val_x = bounds.x;
    if (val_x + val_w > bounds.x + bounds.w) {
      val_x = bounds.x + bounds.w - val_w;
    }
  } else {
    val_x = bounds.x;
  }
  int val_y = bounds.y;
  gfx.drawText(val_x, val_y, buf);

  if (isFocused()) {
    int pad = 2;
    int fx = cx - radius - pad;
    int fy = knob_center_y - radius - pad;
    int fw = (radius + pad) * 2;
    int fh = (radius + pad) * 2;
    if (fw > 0 && fh > 0) {
      gfx.drawRect(fx, fy, fw, fh, kFocusColor);
    }
  }

  // DEBUG: draw boundaries
  // gfx.drawRect(bounds.x, bounds.y, bounds.w, bounds.h, IGfxColor::Red());
}
