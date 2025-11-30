#pragma once
#include "dsp_engine.h"
#include "display.h"

class MiniAcidDisplay {
public:
  MiniAcidDisplay(IGfx& gfx, MiniAcid& mini_acid);
  ~MiniAcidDisplay();
  void update();
  void nextPage();
  void previousPage();
  int active303Voice() const;
  bool is303ControlPage() const;
  void dismissSplash();
  bool showingSplash() const { return splash_active_; }

private:
  enum Page {
    kKnobs = 0,
    kKnobs2,
    kSequencer,
    kDrumSequencer,
    kWaveform,
    kHelp1,
    kHelp2,
    kPageCount
  };

  int drawPageTitle(int x, int y, int w, const char* text);
  void drawWaveform(int x, int y, int w, int h);
  void drawMutesSection(int x, int y, int w, int h);
  void drawHelpPage(int x, int y, int w, int h, int helpPage);
  void drawKnobPage(int x, int y, int w, int h, int voiceIndex);
  void drawSequencerPage(int x, int y, int w, int h);
  void drawDrumSequencerPage(int x, int y, int w, int h);
  void draw303Lane(int x, int y, int w, int h, int voiceIndex);
  void drawDrumLane(int x, int y, int w, int h);
  void drawPageHint(int x, int y);
  void drawSplashScreen();

  IGfx& gfx_;
  MiniAcid& mini_acid_;
  int page_index_ = 0;
  unsigned long splash_start_ms_ = 0;
  bool splash_active_ = true;
};
