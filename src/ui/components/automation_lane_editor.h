#pragma once

#include "../ui_core.h"

struct AutomationLaneAccess {
  std::function<const Parameter&(int param_id)> parameter;
  std::function<const AutomationLane*(int param_id)> lane;
  std::function<AutomationLane*(int param_id)> editLane;
  std::function<int()> currentStep;
  std::function<bool()> isPlaying;
  std::function<float()> currentStepProgress;
};

class AutomationLaneEditor : public FocusableComponent {
 public:
  AutomationLaneEditor(AudioGuard& audio_guard, AutomationLaneAccess access, int param_id);

  void setParamId(int id);
  int paramId() const { return param_id_; }

  bool handleEvent(UIEvent& ui_event) override;
  void draw(IGfx& gfx) override;

 private:
  static constexpr int kDefaultYSteps = 32;
  static constexpr int kXSteps = kAutomationMaxX + 1;
  static constexpr int kGraphPadding = 3;

  void withAudioGuard(const std::function<void()>& fn);
  void clampCursor();
  void setCursorFromPoint(int x, int y);
  uint8_t cursorValue() const;
  int valueToYIndex(uint8_t value) const;
  int ySteps() const;
  Rect graphBounds() const;
  int xToPixel(int x) const;
  int yIndexToPixel(int yIndex) const;
  bool toggleNodeAtCursor();
  bool removeNodeAtCursor();
  bool addNodeAtCursor();

  AudioGuard& audio_guard_;
  AutomationLaneAccess access_;
  int param_id_;
  int cursor_x_;
  int cursor_y_;
};
