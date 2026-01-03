#include "tb303_params_page.h"

#include <cstdarg>
#include <cstdio>

#include "../help_dialog.h"

namespace {
struct Knob {
  const char* label;
  float value;
  float minValue;
  float maxValue;
  const char* unit;

  void draw(IGfx& gfx, int cx, int cy, int radius, IGfxColor ringColor,
            IGfxColor indicatorColor) const {
    float norm = 0.0f;
    if (maxValue > minValue)
      norm = (value - minValue) / (maxValue - minValue);
    norm = std::clamp(norm, 0.0f, 1.0f);

    gfx.drawKnobFace(cx, cy, radius, ringColor, COLOR_BLACK);

    constexpr float kDegToRad = 3.14159265f / 180.0f;

    float degAngle = (135.0f + norm * 270.0f);
    if (degAngle >= 360.0f)
      degAngle -= 360.0f;
    float angle = (degAngle) * kDegToRad;

    int ix = cx + static_cast<int>(roundf(cosf(angle) * (radius - 2)));
    int iy = cy + static_cast<int>(roundf(sinf(angle) * (radius - 2)));

    drawLineColored(gfx, cx, cy, ix, iy, indicatorColor);

    gfx.setTextColor(COLOR_LABEL);
    int label_x = cx - textWidth(gfx, label) / 2;
    gfx.drawText(label_x, cy + radius + 6, label);

    char buf[48];
    if (unit && unit[0]) {
      snprintf(buf, sizeof(buf), "%.0f %s", value, unit);
    } else {
      snprintf(buf, sizeof(buf), "%.2f", value);
    }
    int val_x = cx - textWidth(gfx, buf) / 2;
    gfx.drawText(val_x, cy - radius - 14, buf);
  }
};

inline constexpr IGfxColor kFocusColor = IGfxColor(0xB36A00);
} // namespace

Synth303ParamsPage::Synth303ParamsPage(IGfx& gfx, MiniAcid& mini_acid, AudioGuard& audio_guard, int voice_index) :
    gfx_(gfx),
    mini_acid_(mini_acid),
    audio_guard_(audio_guard),
    voice_index_(voice_index)
{
  title_ = voice_index_ == 0 ? "303A PARAMS" : "303B PARAMS";
}

void Synth303ParamsPage::adjustFocusedElement(int direction) {
  int steps = 5;
  switch (static_cast<FocusTarget>(focus_elements_.focusIndex())) {
    case FocusTarget::Cutoff:
      withAudioGuard([&]() {
        mini_acid_.adjust303Parameter(TB303ParamId::Cutoff, steps * direction, voice_index_);
      });
      break;
    case FocusTarget::Resonance:
      withAudioGuard([&]() {
        mini_acid_.adjust303Parameter(TB303ParamId::Resonance, steps * direction, voice_index_);
      });
      break;
    case FocusTarget::EnvAmount:
      withAudioGuard([&]() {
        mini_acid_.adjust303Parameter(TB303ParamId::EnvAmount, steps * direction, voice_index_);
      });
      break;
    case FocusTarget::EnvDecay: {
      int delta = direction > 0 ? steps : -1;
      withAudioGuard([&]() {
        mini_acid_.adjust303Parameter(TB303ParamId::EnvDecay, delta, voice_index_);
      });
      break;
    }
    case FocusTarget::Oscillator:
      withAudioGuard([&]() {
        mini_acid_.adjust303Parameter(TB303ParamId::Oscillator, direction, voice_index_);
      });
      break;
    case FocusTarget::Delay: {
      bool enabled = mini_acid_.is303DelayEnabled(voice_index_);
      if ((direction > 0 && !enabled) || (direction < 0 && enabled)) {
        withAudioGuard([&]() {
          mini_acid_.toggleDelay303(voice_index_);
        });
      }
      break;
    }
  }
}

void Synth303ParamsPage::draw(IGfx& gfx, int x, int y, int w, int h)
{
  char buf[128];
  auto print = [&](int px, int py, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    gfx_.drawText(px, py, buf);
  };

  int center_y_for_knobs = y + h / 2 - 13;


  int x_margin = -10;
  int usable_w = w - x_margin * 2;


  int radius = 18;
  int spacing = usable_w / 5;
  
  gfx_.drawLine(x + x_margin, y, x + x_margin , h);
  gfx_.drawLine(x + x_margin + usable_w, y, x + x_margin + usable_w, h);
  
  int cx1 = x + x_margin + spacing * 1;
  int cx2 = x + x_margin + spacing * 2;
  int cx3 = x + x_margin + spacing * 3;
  int cx4 = x + x_margin + spacing * 4;

  const Parameter& pCut = mini_acid_.parameter303(TB303ParamId::Cutoff, voice_index_);
  const Parameter& pRes = mini_acid_.parameter303(TB303ParamId::Resonance, voice_index_);
  const Parameter& pEnv = mini_acid_.parameter303(TB303ParamId::EnvAmount, voice_index_);
  const Parameter& pDec = mini_acid_.parameter303(TB303ParamId::EnvDecay, voice_index_);
  const Parameter& pOsc = mini_acid_.parameter303(TB303ParamId::Oscillator, voice_index_);

  bool delayEnabled = mini_acid_.is303DelayEnabled(voice_index_);
  Knob cutoff{pCut.label(), pCut.value(), pCut.min(), pCut.max(), pCut.unit()};
  Knob res{pRes.label(), pRes.value(), pRes.min(), pRes.max(), pRes.unit()};
  Knob env{pEnv.label(), pEnv.value(), pEnv.min(), pEnv.max(), pEnv.unit()};
  Knob dec{pDec.label(), pDec.value(), pDec.min(), pDec.max(), pDec.unit()};

  cutoff.draw(gfx_, cx1, center_y_for_knobs, radius, COLOR_KNOB_1, COLOR_KNOB_1);
  res.draw(gfx_, cx2, center_y_for_knobs, radius, COLOR_KNOB_2, COLOR_KNOB_2);
  env.draw(gfx_, cx3, center_y_for_knobs, radius, COLOR_KNOB_3, COLOR_KNOB_3);
  dec.draw(gfx_, cx4, center_y_for_knobs, radius, COLOR_KNOB_4, COLOR_KNOB_4);

  int delta_y_for_controls = 35;
  int delta_x_for_controls = -9;

  gfx_.setTextColor(COLOR_KNOB_CONTROL);
  print(cx1 + delta_x_for_controls, center_y_for_knobs + delta_y_for_controls, "A/Z");
  print(cx2 + delta_x_for_controls, center_y_for_knobs + delta_y_for_controls, "S/X");
  print(cx3 + delta_x_for_controls, center_y_for_knobs + delta_y_for_controls, "D/C");
  print(cx4 + delta_x_for_controls, center_y_for_knobs + delta_y_for_controls, "F/V");

  // oscillator type control
  const char* oscLabel = pOsc.optionLabel();
  if (!oscLabel) oscLabel = "";
  gfx_.setTextColor(COLOR_WHITE);
  snprintf(buf, sizeof(buf), "OSC:");
  int oscLabelX = x + x_margin + 25;
  int oscSwitchesY = y + h - 13;
  int oscLabelW = textWidth(gfx_, "OSC:");
  int oscValueW = textWidth(gfx_, oscLabel);
  int oscValueMaxW = textWidth(gfx_, "super");
  gfx_.drawText(oscLabelX, oscSwitchesY, buf);

  int oscValueX = oscLabelX + oscLabelW + 3;
  snprintf(buf, sizeof(buf), "%s", oscLabel);
  gfx_.setTextColor(IGfxColor::Cyan());
  gfx_.drawText(oscValueX, oscSwitchesY, buf);

  const char* delayValue = delayEnabled ? "on" : "off";
  gfx_.setTextColor(COLOR_WHITE);
  snprintf(buf, sizeof(buf), "DLY:");
  int delayLabelX = oscValueX + oscValueMaxW + 14;
  gfx_.drawText(delayLabelX, oscSwitchesY, buf);

  int delayLabelW = textWidth(gfx_, "DLY:");
  int delayValueX = delayLabelX + delayLabelW + 3;
  snprintf(buf, sizeof(buf), "%s", delayValue);
  gfx_.setTextColor(IGfxColor::Cyan());
  gfx_.drawText(delayValueX, oscSwitchesY, buf);

  gfx_.setTextColor(COLOR_WHITE);

  int focusPadding = 3;
  focus_elements_.setRect(static_cast<size_t>(FocusTarget::Cutoff),
                          cx1 - radius, center_y_for_knobs - radius,
                          radius * 2, radius * 2);
  focus_elements_.setRect(static_cast<size_t>(FocusTarget::Resonance),
                          cx2 - radius, center_y_for_knobs - radius,
                          radius * 2, radius * 2);
  focus_elements_.setRect(static_cast<size_t>(FocusTarget::EnvAmount),
                          cx3 - radius, center_y_for_knobs - radius,
                          radius * 2, radius * 2);
  focus_elements_.setRect(static_cast<size_t>(FocusTarget::EnvDecay),
                          cx4 - radius, center_y_for_knobs - radius,
                          radius * 2, radius * 2);

  int oscFocusW = oscLabelW + 3 + oscValueW;
  int oscFocusH = gfx_.fontHeight();
  focus_elements_.setRect(static_cast<size_t>(FocusTarget::Oscillator),
                          oscLabelX, oscSwitchesY,
                          oscFocusW, oscFocusH);

  int delayValueW = textWidth(gfx_, delayValue);
  int delayFocusW = delayLabelW + 3 + delayValueW;
  int delayFocusH = gfx_.fontHeight();
  focus_elements_.setRect(static_cast<size_t>(FocusTarget::Delay),
                          delayLabelX, oscSwitchesY,
                          delayFocusW, delayFocusH);

  focus_elements_.drawFocus(gfx_, kFocusColor, focusPadding);
}

void Synth303ParamsPage::withAudioGuard(const std::function<void()>& fn) {
  if (audio_guard_) {
    audio_guard_(fn);
    return;
  }
  fn();
}

const std::string & Synth303ParamsPage::getTitle() const 
{
  return title_;
}

void Synth303ParamsPage::drawHelpBody(IGfx& gfx, int x, int y, int w, int h) {
  if (w <= 0 || h <= 0) return;
  switch (help_page_index_) {
    case 0:
      drawHelpPage303(gfx, x, y, w, h);
      break;
    default:
      break;
  }
  drawHelpScrollbar(gfx, x, y, w, h, help_page_index_, total_help_pages_);
}

bool Synth303ParamsPage::handleHelpEvent(UIEvent& ui_event) {
  if (ui_event.event_type != MINIACID_KEY_DOWN) return false;
  int next = help_page_index_;
  switch (ui_event.scancode) {
    case MINIACID_UP:
      next -= 1;
      break;
    case MINIACID_DOWN:
      next += 1;
      break;
    default:
      return false;
  }
  if (next < 0) next = 0;
  if (next >= total_help_pages_) next = total_help_pages_ - 1;
  help_page_index_ = next;
  return true;
}

bool Synth303ParamsPage::hasHelpDialog() {
  return true;
}

bool Synth303ParamsPage::handleEvent(UIEvent& ui_event) 
{
  if (ui_event.event_type != MINIACID_KEY_DOWN) return false;

  switch (ui_event.scancode) {
    case MINIACID_LEFT:
      focus_elements_.prev();
      return true;
    case MINIACID_RIGHT:
      focus_elements_.next();
      return true;
    case MINIACID_UP:
      adjustFocusedElement(1);
      return true;
    case MINIACID_DOWN:
      adjustFocusedElement(-1);
      return true;
    default:
      break;
  }

  bool event_handled = false;
  int steps = 5;
  switch(ui_event.key){
    case 't':
      withAudioGuard([&]() {
        mini_acid_.adjust303Parameter(TB303ParamId::Oscillator, 1, voice_index_);
      });
      event_handled = true;
      break;
    case 'g':
      withAudioGuard([&]() {
        mini_acid_.adjust303Parameter(TB303ParamId::Oscillator, -1, voice_index_);
      });
      event_handled = true;
      break;
    case 'a':
      withAudioGuard([&]() { 
        mini_acid_.adjust303Parameter(TB303ParamId::Cutoff, steps, voice_index_); 
      });
      event_handled = true;
      break;
    case 'z':
      withAudioGuard([&]() { 
        mini_acid_.adjust303Parameter(TB303ParamId::Cutoff, -steps, voice_index_);
      });
      event_handled = true;
      break;
    case 's':
      withAudioGuard([&]() { 
        mini_acid_.adjust303Parameter(TB303ParamId::Resonance, steps, voice_index_);
      });
      event_handled = true;
      break;
    case 'x':
      withAudioGuard([&]() { 
        mini_acid_.adjust303Parameter(TB303ParamId::Resonance, -steps, voice_index_);
      });
      event_handled = true;
      break;
    case 'd':
      withAudioGuard([&]() { 
        mini_acid_.adjust303Parameter(TB303ParamId::EnvAmount, steps, voice_index_);
      });
      event_handled = true;
      break;
    case 'c':
      withAudioGuard([&]() { 
        mini_acid_.adjust303Parameter(TB303ParamId::EnvAmount, -steps, voice_index_);
      });
      event_handled = true;
      break;
    case 'f':
      withAudioGuard([&]() { 
        mini_acid_.adjust303Parameter(TB303ParamId::EnvDecay, steps, voice_index_);
      });
      event_handled = true;
      break;
    case 'v':
      withAudioGuard([&]() { 
        mini_acid_.adjust303Parameter(TB303ParamId::EnvDecay, -1, voice_index_);
      });
      event_handled = true;
      break;
    case 'm':
      withAudioGuard([&]() { 
        mini_acid_.toggleDelay303(voice_index_);
      });
      break;
    default:
      break;
  }
  return event_handled;
}
