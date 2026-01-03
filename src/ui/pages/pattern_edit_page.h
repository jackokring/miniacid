#pragma once

#include "../ui_core.h"
#include "../ui_colors.h"
#include "../ui_utils.h"

class PatternEditPage : public IPage {
 public:
  PatternEditPage(IGfx& gfx, MiniAcid& mini_acid, AudioGuard& audio_guard, int voice_index);
  void draw(IGfx& gfx, int x, int y, int w, int h) override;
  void drawHelpBody(IGfx& gfx, int x, int y, int w, int h) override;
  bool handleEvent(UIEvent& ui_event) override;
  bool handleHelpEvent(UIEvent& ui_event) override;
  const std::string & getTitle() const override;
  bool hasHelpDialog() override;

  int activePatternCursor() const;
  int activePatternStep() const;
  void setPatternCursor(int cursorIndex);
  void focusPatternRow();
  void focusPatternSteps();
  bool patternRowFocused() const;
  void movePatternCursor(int delta);
  void movePatternCursorVertical(int delta);
  int voiceIndex() const { return voice_index_; }

 private:
  enum class Focus { Steps = 0, PatternRow };

  int clampCursor(int cursorIndex) const;
  int patternIndexFromKey(char key) const;
  void ensureStepFocus();
  void withAudioGuard(const std::function<void()>& fn);

  IGfx& gfx_;
  MiniAcid& mini_acid_;
  AudioGuard& audio_guard_;
  int voice_index_;
  int pattern_edit_cursor_;
  int pattern_row_cursor_;
  Focus focus_;
  int help_page_index_ = 0;
  int total_help_pages_ = 1;
  std::string title_;
};
