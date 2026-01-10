#pragma once

#include <memory>

#include "ui_core.h"

class IAudioRecorder;

class MiniAcidDisplay {
public:
  MiniAcidDisplay(IGfx& gfx, MiniAcid& mini_acid);
  ~MiniAcidDisplay();
  void setAudioGuard(AudioGuard guard);
  void setAudioRecorder(IAudioRecorder* recorder);
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
  bool translateToApplicationEvent(UIEvent& event);

  IGfx& gfx_;
  MiniAcid& mini_acid_;
  int page_index_ = 0;
  unsigned long splash_start_ms_ = 0;
  bool splash_active_ = true;
  bool help_dialog_visible_ = false;
  std::unique_ptr<MultiPageHelpDialog> help_dialog_;

  AudioGuard audio_guard_;
  IAudioRecorder* audio_recorder_ = nullptr;
  std::vector<std::unique_ptr<IPage>> pages_;
};
