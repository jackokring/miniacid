#pragma once

#include <array>
#include <cstddef>

#include "display.h"

struct FocusRect {
  int x = 0;
  int y = 0;
  int w = 0;
  int h = 0;
};

template <size_t N>
class FocusableElements {
 public:
  FocusableElements() : focus_index_(0) {
    rects_.fill(FocusRect{0, 0, 0, 0});
  }

  void next() {
    focus_index_ = (focus_index_ + 1) % N;
  }

  void prev() {
    focus_index_ = (focus_index_ + N - 1) % N;
  }

  size_t focusIndex() const {
    return focus_index_;
  }

  void setFocusIndex(size_t index) {
    if (index >= N) return;
    focus_index_ = index;
  }

  void setRect(size_t index, int x, int y, int w, int h) {
    if (index >= N) return;
    rects_[index] = {x, y, w, h};
  }

  FocusRect focusRect() const {
    return rects_[focus_index_];
  }

  void drawFocus(IGfx& gfx, IGfxColor color, int padding = 2) const {
    FocusRect rect = focusRect();
    if (rect.w <= 0 || rect.h <= 0) return;
    gfx.drawRect(rect.x - padding, rect.y - padding, rect.w + padding * 2,
                 rect.h + padding * 2, color);
  }

 private:
  std::array<FocusRect, N> rects_;
  size_t focus_index_;
};
