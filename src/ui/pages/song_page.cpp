#include "song_page.h"

#include <cctype>
#include <cstdio>

#include "../help_dialog.h"

SongPage::SongPage(IGfx& gfx, MiniAcid& mini_acid, AudioGuard& audio_guard)
  : gfx_(gfx),
    mini_acid_(mini_acid),
    audio_guard_(audio_guard),
    cursor_row_(0),
    cursor_track_(0),
    scroll_row_(0) {
  cursor_row_ = mini_acid_.currentSongPosition();
  if (cursor_row_ < 0) cursor_row_ = 0;
  int maxSongRow = mini_acid_.songLength() - 1;
  if (maxSongRow < 0) maxSongRow = 0;
  if (cursor_row_ > maxSongRow) cursor_row_ = maxSongRow;
  if (cursor_row_ >= Song::kMaxPositions) cursor_row_ = Song::kMaxPositions - 1;
}

int SongPage::clampCursorRow(int row) const {
  int maxRow = Song::kMaxPositions - 1;
  if (maxRow < 0) maxRow = 0;
  if (row < 0) row = 0;
  if (row > maxRow) row = maxRow;
  return row;
}

int SongPage::cursorRow() const {
  return clampCursorRow(cursor_row_);
}

int SongPage::cursorTrack() const {
  int track = cursor_track_;
  if (track < 0) track = 0;
  if (track > 4) track = 4;
  return track;
}

bool SongPage::cursorOnModeButton() const { return cursorTrack() == 4; }
bool SongPage::cursorOnPlayheadLabel() const { return cursorTrack() == 3; }

void SongPage::moveCursorHorizontal(int delta) {
  int track = cursorTrack();
  track += delta;
  if (track < 0) track = 0;
  if (track > 4) track = 4;
  cursor_track_ = track;
  syncSongPositionToCursor();
}

void SongPage::moveCursorVertical(int delta) {
  if (delta == 0) return;
  if (cursorOnPlayheadLabel() || cursorOnModeButton()) {
    moveCursorHorizontal(delta);
    return;
  }
  int row = cursorRow();
  row += delta;
  row = clampCursorRow(row);
  cursor_row_ = row;
  syncSongPositionToCursor();
}

void SongPage::syncSongPositionToCursor() {
  if (mini_acid_.songModeEnabled() && !mini_acid_.isPlaying()) {
    withAudioGuard([&]() { mini_acid_.setSongPosition(cursorRow()); });
  }
}

void SongPage::withAudioGuard(const std::function<void()>& fn) {
  if (audio_guard_) {
    audio_guard_(fn);
    return;
  }
  fn();
}

SongTrack SongPage::trackForColumn(int col, bool& valid) const {
  valid = true;
  if (col == 0) return SongTrack::SynthA;
  if (col == 1) return SongTrack::SynthB;
  if (col == 2) return SongTrack::Drums;
  valid = false;
  return SongTrack::Drums;
}

int SongPage::patternIndexFromKey(char key) const {
  switch (std::tolower(static_cast<unsigned char>(key))) {
    case 'q': return 0;
    case 'w': return 1;
    case 'e': return 2;
    case 'r': return 3;
    case 't': return 4;
    case 'y': return 5;
    case 'u': return 6;
    case 'i': return 7;
    default: return -1;
  }
}

bool SongPage::adjustSongPatternAtCursor(int delta) {
  bool trackValid = false;
  SongTrack track = trackForColumn(cursorTrack(), trackValid);
  if (!trackValid) return false;
  int row = cursorRow();
  int current = mini_acid_.songPatternAt(row, track);
  int maxPattern = track == SongTrack::Drums
                     ? Bank<DrumPatternSet>::kPatterns - 1
                     : Bank<SynthPattern>::kPatterns - 1;
  int next = current;
  if (delta > 0) next = current < 0 ? 0 : current + 1;
  else if (delta < 0) next = current < 0 ? -1 : current - 1;
  if (next > maxPattern) next = maxPattern;
  if (next < -1) next = -1;
  if (next == current) return false;
  withAudioGuard([&]() {
    if (next < 0) mini_acid_.clearSongPattern(row, track);
    else mini_acid_.setSongPattern(row, track, next);
    if (mini_acid_.songModeEnabled() && !mini_acid_.isPlaying()) {
      mini_acid_.setSongPosition(row);
    }
  });
  return true;
}

bool SongPage::adjustSongPlayhead(int delta) {
  int len = mini_acid_.songLength();
  if (len < 1) len = 1;
  int maxPos = len - 1;
  if (maxPos < 0) maxPos = 0;
  if (maxPos >= Song::kMaxPositions) maxPos = Song::kMaxPositions - 1;
  int current = mini_acid_.songPlayheadPosition();
  int next = current + delta;
  if (next < 0) next = 0;
  if (next > maxPos) next = maxPos;
  if (next == current) return false;
  withAudioGuard([&]() { mini_acid_.setSongPosition(next); });
  setScrollToPlayhead(next);
  return true;
}

bool SongPage::assignPattern(int patternIdx) {
  bool trackValid = false;
  SongTrack track = trackForColumn(cursorTrack(), trackValid);
  if (!trackValid || cursorOnModeButton()) return false;
  int row = cursorRow();
  withAudioGuard([&]() {
    mini_acid_.setSongPattern(row, track, patternIdx);
    if (mini_acid_.songModeEnabled() && !mini_acid_.isPlaying()) {
      mini_acid_.setSongPosition(row);
    }
  });
  return true;
}

bool SongPage::clearPattern() {
  bool trackValid = false;
  SongTrack track = trackForColumn(cursorTrack(), trackValid);
  if (!trackValid) return false;
  int row = cursorRow();
  withAudioGuard([&]() {
    mini_acid_.clearSongPattern(row, track);
    if (mini_acid_.songModeEnabled() && !mini_acid_.isPlaying()) {
      mini_acid_.setSongPosition(row);
    }
  });
  return true;
}

bool SongPage::toggleSongMode() {
  withAudioGuard([&]() { mini_acid_.toggleSongMode(); });
  return true;
}

void SongPage::setScrollToPlayhead(int playhead) {
  if (playhead < 0) playhead = 0;
  int rowHeight = gfx_.fontHeight() + 6;
  if (rowHeight < 8) rowHeight = 8;
  int visibleRows = (gfx_.height() - 20) / rowHeight;
  if (visibleRows < 1) visibleRows = 1;
  if (scroll_row_ > playhead) scroll_row_ = playhead;
  if (scroll_row_ + visibleRows - 1 < playhead) {
    scroll_row_ = playhead - visibleRows + 1;
    if (scroll_row_ < 0) scroll_row_ = 0;
  }
}

bool SongPage::handleEvent(UIEvent& ui_event) {
  if (ui_event.event_type != MINIACID_KEY_DOWN) return false;

  if (ui_event.alt && (ui_event.scancode == MINIACID_UP || ui_event.scancode == MINIACID_DOWN)) {
    int delta = ui_event.scancode == MINIACID_UP ? 1 : -1;
    if (cursorOnPlayheadLabel()) return adjustSongPlayhead(delta);
    return adjustSongPatternAtCursor(delta);
  }

  bool handled = false;
  switch (ui_event.scancode) {
    case MINIACID_LEFT:
      moveCursorHorizontal(-1);
      handled = true;
      break;
    case MINIACID_RIGHT:
      moveCursorHorizontal(1);
      handled = true;
      break;
    case MINIACID_UP:
      moveCursorVertical(-1);
      handled = true;
      break;
    case MINIACID_DOWN:
      moveCursorVertical(1);
      handled = true;
      break;
    default:
      break;
  }
  if (handled) return true;

  char key = ui_event.key;
  if (!key) return false;

  if (cursorOnModeButton() && (key == '\n' || key == '\r')) {
    return toggleSongMode();
  }

  if (key == 'm' || key == 'M') {
    return toggleSongMode();
  }

  int patternIdx = patternIndexFromKey(key);
  if (cursorOnModeButton() && patternIdx >= 0) return false;
  if (patternIdx >= 0) return assignPattern(patternIdx);

  if (key == '\b') {
    return clearPattern();
  }

  return false;
}

const std::string & SongPage::getTitle() const {
  static std::string title = "SONG";
  return title;
}

void SongPage::drawHelpBody(IGfx& gfx, int x, int y, int w, int h) {
  if (w <= 0 || h <= 0) return;
  switch (help_page_index_) {
    case 0:
      drawHelpPageSong(gfx, x, y, w, h);
      break;
    case 1:
      drawHelpPageSongCont(gfx, x, y, w, h);
      break;
    default:
      break;
  }
  drawHelpScrollbar(gfx, x, y, w, h, help_page_index_, total_help_pages_);
}

bool SongPage::handleHelpEvent(UIEvent& ui_event) {
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

bool SongPage::hasHelpDialog() {
  return true;
}

void SongPage::draw(IGfx& gfx, int x, int y, int w, int h) {
  int body_y = y + 2;
  int body_h = h - 2;
  if (body_h <= 0) return;

  int label_h = gfx.fontHeight();
  int header_h = label_h + 4;
  int row_h = label_h + 6;
  if (row_h < 10) row_h = 10;
  int usable_h = body_h - header_h;
  if (usable_h < row_h) usable_h = row_h;
  int visible_rows = usable_h / row_h;
  if (visible_rows < 1) visible_rows = 1;

  int song_len = mini_acid_.songLength();
  int cursor_row = cursorRow();
  int playhead = mini_acid_.songPlayheadPosition();
  bool playingSong = mini_acid_.isPlaying() && mini_acid_.songModeEnabled();

  if (playingSong) {
    int minTarget = std::min(cursor_row, playhead);
    int maxTarget = std::max(cursor_row, playhead);
    if (minTarget < scroll_row_) scroll_row_ = minTarget;
    if (maxTarget >= scroll_row_ + visible_rows) scroll_row_ = maxTarget - visible_rows + 1;
  } else {
    if (cursor_row < scroll_row_) scroll_row_ = cursor_row;
    if (cursor_row >= scroll_row_ + visible_rows) scroll_row_ = cursor_row - visible_rows + 1;
  }
  if (scroll_row_ < 0) scroll_row_ = 0;
  int maxStart = Song::kMaxPositions - visible_rows;
  if (maxStart < 0) maxStart = 0;
  if (scroll_row_ > maxStart) scroll_row_ = maxStart;

  int pos_col_w = 20;
  int spacing = 3;
  int modeBtnW = 70;
  int track_col_w = (w - pos_col_w - spacing * 5 - modeBtnW) / 3;
  if (track_col_w < 20) track_col_w = 20;

  gfx.setTextColor(COLOR_LABEL);
  gfx.drawText(x, body_y, "POS");
  gfx.drawText(x + pos_col_w + spacing, body_y, "303A");
  gfx.drawText(x + pos_col_w + spacing + track_col_w, body_y, "303B");
  gfx.drawText(x + pos_col_w + spacing + track_col_w * 2, body_y, "Drums");
  char lenBuf[24];
  snprintf(lenBuf, sizeof(lenBuf), "PLAYHD %d:%d", playhead + 1, song_len);
  int lenX = x + pos_col_w + spacing + track_col_w * 3 + spacing + 10;
  int lenW = textWidth(gfx, lenBuf);
  bool playheadSelected = cursorOnPlayheadLabel();
  if (playheadSelected) {
    gfx.drawRect(lenX - 2, body_y - 1, lenW + 4, label_h + 2, COLOR_STEP_SELECTED);
  }
  gfx.drawText(lenX, body_y, lenBuf);

  bool songMode = mini_acid_.songModeEnabled();
  IGfxColor modeColor = songMode ? IGfxColor::Green() : IGfxColor::Blue();
  int modeX = x + w - modeBtnW;
  int modeY = body_y - 2 + 30;
  int modeH = header_h + row_h;
  gfx.fillRect(modeX, modeY, modeBtnW - 2, modeH, COLOR_PANEL);
  gfx.drawRect(modeX, modeY, modeBtnW - 2, modeH, modeColor);
  char modeLabel[32];
  snprintf(modeLabel, sizeof(modeLabel), "MODE:%s", songMode ? "SONG" : "PAT");
  int twMode = textWidth(gfx, modeLabel);
  gfx.setTextColor(modeColor);
  gfx.drawText(modeX + (modeBtnW - twMode) / 2, modeY + modeH / 2 - label_h / 2, modeLabel);
  gfx.setTextColor(COLOR_WHITE);
  bool modeSelected = cursorOnModeButton();
  if (modeSelected) {
    gfx.drawRect(modeX - 2, modeY - 2, modeBtnW + 2, modeH + 4, COLOR_STEP_SELECTED);
  }

  int row_y = body_y + header_h;
  for (int i = 0; i < visible_rows; ++i) {
    int row_idx = scroll_row_ + i;
    if (row_idx >= Song::kMaxPositions) break;
    bool isCursorRow = row_idx == cursor_row;
    bool isPlayhead = playingSong && row_idx == playhead;
    IGfxColor rowHighlight = isPlayhead ? IGfxColor::Magenta() : COLOR_DARKER;
    if (isPlayhead) {
      gfx.fillRect(x, row_y - 1, w - modeBtnW - 2, row_h, rowHighlight);
    } else if (isCursorRow) {
      gfx.fillRect(x, row_y - 1, w - modeBtnW - 2, row_h, COLOR_PANEL);
    } else {
      gfx.fillRect(x, row_y - 1, w - modeBtnW - 2, row_h, COLOR_DARKER);
    }

    char posLabel[8];
    snprintf(posLabel, sizeof(posLabel), "%d", row_idx + 1);
    gfx.setTextColor(row_idx < song_len ? COLOR_WHITE : COLOR_LABEL);
    gfx.drawText(x, row_y + 2, posLabel);
    gfx.setTextColor(COLOR_WHITE);

    for (int t = 0; t < SongPosition::kTrackCount; ++t) {
      int col_x = x + pos_col_w + spacing + t * (track_col_w + spacing);
      int patternIdx = mini_acid_.songPatternAt(row_idx,
                        t == 0 ? SongTrack::SynthA : (t == 1 ? SongTrack::SynthB : SongTrack::Drums));
      bool isSelected = isCursorRow && cursorTrack() == t;
      if (isSelected) {
        gfx.drawRect(col_x - 1, row_y - 2, track_col_w + 2, row_h + 2 - 1, COLOR_STEP_SELECTED);
      }
      char label[6];
      if (patternIdx < 0) {
        snprintf(label, sizeof(label), "--");
        gfx.setTextColor(COLOR_LABEL);
      } else {
        snprintf(label, sizeof(label), "%d", patternIdx + 1);
        gfx.setTextColor(COLOR_WHITE);
      }
      int tw = textWidth(gfx, label);
      int tx = col_x + (track_col_w - tw) / 2;
      gfx.drawText(tx, row_y + (row_h - label_h) / 2 - 1, label);
      gfx.setTextColor(COLOR_WHITE);
    }
    row_y += row_h;
  }
}
