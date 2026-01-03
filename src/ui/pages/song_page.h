#pragma once

#include "../ui_core.h"
#include "../ui_colors.h"
#include "../ui_utils.h"

class SongPage : public IPage{
 public:
  SongPage(IGfx& gfx, MiniAcid& mini_acid, AudioGuard& audio_guard);
  void draw(IGfx& gfx, int x, int y, int w, int h) override;
  void drawHelpBody(IGfx& gfx, int x, int y, int w, int h) override;
  bool handleEvent(UIEvent& ui_event) override;
  bool handleHelpEvent(UIEvent& ui_event) override;
  const std::string & getTitle() const override;
  bool hasHelpDialog() override;

  void setScrollToPlayhead(int playhead);
 private:
  int clampCursorRow(int row) const;
  int cursorRow() const;
  int cursorTrack() const;
  bool cursorOnModeButton() const;
  bool cursorOnPlayheadLabel() const;
  void moveCursorHorizontal(int delta);
  void moveCursorVertical(int delta);
  void syncSongPositionToCursor();
  void withAudioGuard(const std::function<void()>& fn);
  SongTrack trackForColumn(int col, bool& valid) const;
  int patternIndexFromKey(char key) const;
  bool adjustSongPatternAtCursor(int delta);
  bool adjustSongPlayhead(int delta);
  bool assignPattern(int patternIdx);
  bool clearPattern();
  bool toggleSongMode();

  IGfx& gfx_;
  MiniAcid& mini_acid_;
  AudioGuard& audio_guard_;
  int cursor_row_;
  int cursor_track_;
  int scroll_row_;
  int help_page_index_ = 0;
  int total_help_pages_ = 2;
};
