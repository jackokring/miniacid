#pragma once

#include "../ui_core.h"
#include "../ui_colors.h"
#include "../ui_utils.h"
#include "../focusable_elements.h"

class Synth303ParamsPage : public IPage {
 public:
  Synth303ParamsPage(IGfx& gfx, MiniAcid& mini_acid, AudioGuard& audio_guard, int voice_index);
  void draw(IGfx& gfx, int x, int y, int w, int h) override;
  void drawHelpBody(IGfx& gfx, int x, int y, int w, int h) override;
  bool handleEvent(UIEvent& ui_event) override;
  bool handleHelpEvent(UIEvent& ui_event) override;
 const std::string & getTitle() const override;
  bool hasHelpDialog() override;

 private:
  enum class FocusTarget {
    Cutoff = 0,
    Resonance,
    EnvAmount,
    EnvDecay,
    Oscillator,
    Delay,
  };

  void withAudioGuard(const std::function<void()>& fn);
  void adjustFocusedElement(int direction);

  IGfx& gfx_;
  MiniAcid& mini_acid_;
  AudioGuard& audio_guard_;
  int voice_index_;
  FocusableElements<6> focus_elements_;
  int help_page_index_ = 0;
  int total_help_pages_ = 1;
  std::string title_;
};
