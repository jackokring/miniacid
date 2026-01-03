#pragma once

#include <memory>

#include "ui_core.h"

class MiniAcidDisplay {
public:
  MiniAcidDisplay(IGfx& gfx, MiniAcid& mini_acid);
  ~MiniAcidDisplay();
  void setAudioGuard(AudioGuard guard);
  void update();
  void nextPage();
  void previousPage();
  void dismissSplash();
  bool handleEvent(UIEvent event);

private:

  void drawMutesSection(int x, int y, int w, int h);
  int drawPageTitle(int x, int y, int w, const char* text);
  void drawPageHint(int x, int y);
  void drawSplashScreen();
  void drawHelpDialog(IPage& page, int x, int y, int w, int h);

  IGfx& gfx_;
  MiniAcid& mini_acid_;
  int page_index_ = 0;
  unsigned long splash_start_ms_ = 0;
  bool splash_active_ = true;
  bool help_dialog_visible_ = false;

  AudioGuard audio_guard_;
  std::vector<std::unique_ptr<IPage>> pages_;
};
