#include "pattern_edit_page.h"

#include <cctype>
#include <cstdio>

#include "../help_dialog.h"

PatternEditPage::PatternEditPage(IGfx& gfx, MiniAcid& mini_acid, AudioGuard& audio_guard, int voice_index)
  : gfx_(gfx),
    mini_acid_(mini_acid),
    audio_guard_(audio_guard),
    voice_index_(voice_index),
    pattern_edit_cursor_(0),
    pattern_row_cursor_(0),
    focus_(Focus::Steps) {
  int idx = mini_acid_.current303PatternIndex(voice_index_);
  if (idx < 0 || idx >= Bank<SynthPattern>::kPatterns) idx = 0;
  pattern_row_cursor_ = idx;
  title_ = voice_index_ == 0 ? "303A PATTERNS" : "303B PATTERNS";
}

int PatternEditPage::clampCursor(int cursorIndex) const {
  int cursor = cursorIndex;
  if (cursor < 0) cursor = 0;
  if (cursor >= Bank<SynthPattern>::kPatterns)
    cursor = Bank<SynthPattern>::kPatterns - 1;
  return cursor;
}

int PatternEditPage::patternIndexFromKey(char key) const {
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

void PatternEditPage::ensureStepFocus() {
  if (patternRowFocused()) focus_ = Focus::Steps;
}

void PatternEditPage::withAudioGuard(const std::function<void()>& fn) {
  if (audio_guard_) {
    audio_guard_(fn);
    return;
  }
  fn();
}

int PatternEditPage::activePatternCursor() const {
  return clampCursor(pattern_row_cursor_);
}

int PatternEditPage::activePatternStep() const {
  int idx = pattern_edit_cursor_;
  if (idx < 0) idx = 0;
  if (idx >= SEQ_STEPS) idx = SEQ_STEPS - 1;
  return idx;
}

void PatternEditPage::setPatternCursor(int cursorIndex) {
  pattern_row_cursor_ = clampCursor(cursorIndex);
}

void PatternEditPage::focusPatternRow() {
  if (mini_acid_.songModeEnabled()) return;
  setPatternCursor(pattern_row_cursor_);
  focus_ = Focus::PatternRow;
}

void PatternEditPage::focusPatternSteps() {
  int row = pattern_edit_cursor_ / 8;
  if (row < 0 || row > 1) row = 0;
  pattern_edit_cursor_ = row * 8 + activePatternCursor();
  focus_ = Focus::Steps;
}

bool PatternEditPage::patternRowFocused() const {
  if (mini_acid_.songModeEnabled()) return false;
  return focus_ == Focus::PatternRow;
}

void PatternEditPage::movePatternCursor(int delta) {
  if (mini_acid_.songModeEnabled() && focus_ == Focus::PatternRow) {
    focus_ = Focus::Steps;
  }
  if (focus_ == Focus::PatternRow) {
    int cursor = activePatternCursor();
    cursor = (cursor + delta) % Bank<SynthPattern>::kPatterns;
    if (cursor < 0) cursor += Bank<SynthPattern>::kPatterns;
    pattern_row_cursor_ = cursor;
    return;
  }
  int idx = activePatternStep();
  int row = idx / 8;
  int col = idx % 8;
  col = (col + delta) % 8;
  if (col < 0) col += 8;
  pattern_edit_cursor_ = row * 8 + col;
}

void PatternEditPage::movePatternCursorVertical(int delta) {
  if (delta == 0) return;
  if (mini_acid_.songModeEnabled() && focus_ == Focus::PatternRow) {
    focus_ = Focus::Steps;
  }
  if (focus_ == Focus::PatternRow) {
    int col = activePatternCursor();
    int targetRow = delta > 0 ? 0 : 1;
    pattern_edit_cursor_ = targetRow * 8 + col;
    focus_ = Focus::Steps;
    return;
  }
  int idx = activePatternStep();
  int row = idx / 8;
  int col = idx % 8;
  int newRow = row + delta;
  if (newRow < 0) {
    if (mini_acid_.songModeEnabled()) newRow = 0;
    else {
      focus_ = Focus::PatternRow;
      setPatternCursor(col);
      return;
    }
  }
  if (newRow > 1) {
    if (mini_acid_.songModeEnabled()) newRow = 1;
    else {
      focus_ = Focus::PatternRow;
      setPatternCursor(col);
      return;
    }
  }
  pattern_edit_cursor_ = newRow * 8 + col;
}

const std::string & PatternEditPage::getTitle() const {
  return title_;
}

void PatternEditPage::drawHelpBody(IGfx& gfx, int x, int y, int w, int h) {
  if (w <= 0 || h <= 0) return;
  switch (help_page_index_) {
    case 0:
      drawHelpPage303PatternEdit(gfx, x, y, w, h);
      break;
    default:
      break;
  }
  drawHelpScrollbar(gfx, x, y, w, h, help_page_index_, total_help_pages_);
}

bool PatternEditPage::handleHelpEvent(UIEvent& ui_event) {
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

bool PatternEditPage::hasHelpDialog() {
  return true;
}

bool PatternEditPage::handleEvent(UIEvent& ui_event) {
  if (ui_event.event_type != MINIACID_KEY_DOWN) return false;
  bool handled = false;
  switch (ui_event.scancode) {
    case MINIACID_LEFT:
      movePatternCursor(-1);
      handled = true;
      break;
    case MINIACID_RIGHT:
      movePatternCursor(1);
      handled = true;
      break;
    case MINIACID_UP:
      movePatternCursorVertical(-1);
      handled = true;
      break;
    case MINIACID_DOWN:
      movePatternCursorVertical(1);
      handled = true;
      break;
    default:
      break;
  }
  if (handled) return true;

  char key = ui_event.key;
  if (!key) return false;

  if ((key == '\n' || key == '\r') && patternRowFocused()) {
    if (mini_acid_.songModeEnabled()) return true;
    int cursor = activePatternCursor();
    setPatternCursor(cursor);
    withAudioGuard([&]() { mini_acid_.set303PatternIndex(voice_index_, cursor); });
    return true;
  }

  int patternIdx = patternIndexFromKey(key);
  bool patternKeyReserved = false;
  if (patternIdx >= 0) {
    char lowerKey = static_cast<char>(std::tolower(static_cast<unsigned char>(key)));
    patternKeyReserved = (lowerKey == 'q' || lowerKey == 'w');
    if (!patternKeyReserved || patternRowFocused()) {
      if (mini_acid_.songModeEnabled()) return true;
      focusPatternRow();
      setPatternCursor(patternIdx);
      withAudioGuard([&]() { mini_acid_.set303PatternIndex(voice_index_, patternIdx); });
      return true;
    }
  }

  auto ensureStepFocusAndCursor = [&]() {
    if (patternRowFocused()) {
      focusPatternSteps();
    } else {
      ensureStepFocus();
    }
  };

  char lowerKey = static_cast<char>(std::tolower(static_cast<unsigned char>(key)));
  switch (lowerKey) {
    case 'q': {
      ensureStepFocusAndCursor();
      int step = activePatternStep();
      withAudioGuard([&]() { mini_acid_.toggle303SlideStep(voice_index_, step); });
      return true;
    }
    case 'w': {
      ensureStepFocusAndCursor();
      int step = activePatternStep();
      withAudioGuard([&]() { mini_acid_.toggle303AccentStep(voice_index_, step); });
      return true;
    }
    case 'a': {
      ensureStepFocusAndCursor();
      int step = activePatternStep();
      withAudioGuard([&]() { mini_acid_.adjust303StepNote(voice_index_, step, 1); });
      return true;
    }
    case 'z': {
      ensureStepFocusAndCursor();
      int step = activePatternStep();
      withAudioGuard([&]() { mini_acid_.adjust303StepNote(voice_index_, step, -1); });
      return true;
    }
    case 's': {
      ensureStepFocusAndCursor();
      int step = activePatternStep();
      withAudioGuard([&]() { mini_acid_.adjust303StepOctave(voice_index_, step, 1); });
      return true;
    }
    case 'x': {
      ensureStepFocusAndCursor();
      int step = activePatternStep();
      withAudioGuard([&]() { mini_acid_.adjust303StepOctave(voice_index_, step, -1); });
      return true;
    }
    default:
      break;
  }

  if (key == '\b') {
    ensureStepFocusAndCursor();
    int step = activePatternStep();
    withAudioGuard([&]() { mini_acid_.clear303StepNote(voice_index_, step); });
    return true;
  }

  return false;
}

void PatternEditPage::draw(IGfx& gfx, int x, int y, int w, int h) {

  int body_y = y + 2;
  int body_h = h - 2;
  if (body_h <= 0) return;

  const int8_t* notes = mini_acid_.pattern303Steps(voice_index_);
  const bool* accent = mini_acid_.pattern303AccentSteps(voice_index_);
  const bool* slide = mini_acid_.pattern303SlideSteps(voice_index_);
  int stepCursor = pattern_edit_cursor_;
  int playing = mini_acid_.currentStep();
  int selectedPattern = mini_acid_.display303PatternIndex(voice_index_);
  bool songMode = mini_acid_.songModeEnabled();
  bool patternFocus = !songMode && patternRowFocused();
  bool stepFocus = !patternFocus;
  int patternCursor = songMode && selectedPattern >= 0 ? selectedPattern : activePatternCursor();

  int spacing = 4;
  int pattern_size = (w - spacing * 7 - 2) / 8;
  int pattern_size_height = pattern_size / 2;
  if (pattern_size < 12) pattern_size = 12;
  int pattern_label_h = gfx.fontHeight();
  int pattern_row_y = body_y + pattern_label_h + 1;

  gfx.setTextColor(COLOR_LABEL);
  gfx.drawText(x, body_y, "PATTERNS");
  gfx.setTextColor(COLOR_WHITE);

  for (int i = 0; i < Bank<SynthPattern>::kPatterns; ++i) {
    int col = i % 8;
    int cell_x = x + col * (pattern_size + spacing);
    bool isCursor = patternFocus && patternCursor == i;
    IGfxColor bg = songMode ? COLOR_GRAY_DARKER : COLOR_PANEL;
    gfx.fillRect(cell_x, pattern_row_y, pattern_size, pattern_size_height, bg);
    if (selectedPattern == i) {
      IGfxColor sel = songMode ? IGfxColor::Yellow() : COLOR_PATTERN_SELECTED_FILL;
      IGfxColor border = songMode ? IGfxColor::Yellow() : COLOR_LABEL;
      gfx.fillRect(cell_x - 1, pattern_row_y - 1, pattern_size + 2, pattern_size_height + 2, sel);
      gfx.drawRect(cell_x - 1, pattern_row_y - 1, pattern_size + 2, pattern_size_height + 2, border);
    }
    gfx.drawRect(cell_x, pattern_row_y, pattern_size, pattern_size_height, songMode ? COLOR_LABEL : COLOR_WHITE);
    if (isCursor) {
      gfx.drawRect(cell_x - 2, pattern_row_y - 2, pattern_size + 4, pattern_size_height + 4, COLOR_STEP_SELECTED);
    }
    char label[4];
    snprintf(label, sizeof(label), "%d", i + 1);
    int tw = textWidth(gfx, label);
    int tx = cell_x + (pattern_size - tw) / 2;
    int ty = pattern_row_y + pattern_size_height / 2 - gfx.fontHeight() / 2;
    gfx.setTextColor(songMode ? COLOR_LABEL : COLOR_WHITE);
    gfx.drawText(tx, ty, label);
    gfx.setTextColor(COLOR_WHITE);
  }

  int grid_top = pattern_row_y + pattern_size_height + 6;
  int cell_size = (w - spacing * 7 - 2) / 8;
  if (cell_size < 12) cell_size = 12;
  int indicator_h = 5;
  int indicator_gap = 1;
  int row_height = indicator_h + indicator_gap + cell_size + 4;

  for (int i = 0; i < SEQ_STEPS; ++i) {
    int row = i / 8;
    int col = i % 8;
    int cell_x = x + col * (cell_size + spacing);
    int cell_y = grid_top + row * row_height;

    int indicator_w = (cell_size - 2) / 2;
    if (indicator_w < 4) indicator_w = 4;
    int slide_x = cell_x + cell_size - indicator_w;
    int indicator_y = cell_y;

    gfx.fillRect(cell_x, indicator_y, indicator_w, indicator_h, slide[i] ? COLOR_SLIDE : COLOR_GRAY_DARKER);
    gfx.drawRect(cell_x, indicator_y, indicator_w, indicator_h, COLOR_WHITE);
    gfx.fillRect(slide_x, indicator_y, indicator_w, indicator_h, accent[i] ? COLOR_ACCENT : COLOR_GRAY_DARKER);
    gfx.drawRect(slide_x, indicator_y, indicator_w, indicator_h, COLOR_WHITE);

    int note_box_y = indicator_y + indicator_h + indicator_gap;
    IGfxColor fill = notes[i] >= 0 ? COLOR_303_NOTE : COLOR_GRAY;
    gfx.fillRect(cell_x, note_box_y, cell_size, cell_size, fill);
    gfx.drawRect(cell_x, note_box_y, cell_size, cell_size, COLOR_WHITE);

    if (playing == i) {
      gfx.drawRect(cell_x - 1, note_box_y - 1, cell_size + 2, cell_size + 2, COLOR_STEP_HILIGHT);
    }
    if (stepFocus && stepCursor == i) {
      gfx.drawRect(cell_x - 2, note_box_y - 2, cell_size + 4, cell_size + 4, COLOR_STEP_SELECTED);
    }

    char note_label[8];
    formatNoteName(notes[i], note_label, sizeof(note_label));
    int tw = textWidth(gfx, note_label);
    int tx = cell_x + (cell_size - tw) / 2;
    int ty = note_box_y + cell_size / 2 - gfx.fontHeight() / 2;
    gfx.drawText(tx, ty, note_label);
  }
}
