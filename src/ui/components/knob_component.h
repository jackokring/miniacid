#pragma once

#include "../ui_core.h"

struct KnobAutomationAccess {
  std::function<const AutomationLane*(int param_id)> lane;
};

class KnobComponent : public FocusableComponent {
 public:
  KnobComponent(const Parameter& param, IGfxColor ring_color,
                IGfxColor indicator_color,
                std::function<void(int)> adjust_fn,
                KnobAutomationAccess access = {}, int param_id = -1);

  void setValue(int direction);
  bool handleEvent(UIEvent& ui_event) override;
  void draw(IGfx& gfx) override;

 private:
  const Parameter& param_;
  IGfxColor ring_color_;
  IGfxColor indicator_color_;
  std::function<void(int)> adjust_fn_;
  bool dragging_ = false;
  int last_drag_y_ = 0;
  int drag_accum_ = 0;
  KnobAutomationAccess access_;
  int param_id_ = -1;
  int last_lut_index_ = -1;
  int last_radius_ = -1;
  int last_ix_ = 0;
  int last_iy_ = 0;
};
