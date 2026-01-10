#pragma once

#include "../ui_core.h"
#include "../pages/help_dialog.h"
#include "../ui_colors.h"
#include "../ui_utils.h"

class BankSelectionBarComponent;
class PatternSelectionBarComponent;

class DrumSequencerPage : public IPage, public IMultiHelpFramesProvider {
 public:
  DrumSequencerPage(IGfx& gfx, MiniAcid& mini_acid, AudioGuard& audio_guard);
  void draw(IGfx& gfx) override;
  bool handleEvent(UIEvent& ui_event) override;
  const std::string & getTitle() const override;
  std::unique_ptr<MultiPageHelpDialog> getHelpDialog() override;
 int getHelpFrameCount() const override;
  void drawHelpFrame(IGfx& gfx, int frameIndex, Rect bounds) const override;

 private:
  int activeDrumPatternCursor() const;
  int activeDrumStep() const;
  int activeDrumVoice() const;
  int activeBankCursor() const;
  void setDrumPatternCursor(int cursorIndex);
  void moveDrumCursor(int delta);
  void moveDrumCursorVertical(int delta);
  void focusPatternRow();
  void focusGrid();
  bool patternRowFocused() const;
  int patternIndexFromKey(char key) const;
  int bankIndexFromKey(char key) const;
  void setBankIndex(int bankIndex);
  bool bankRowFocused() const;
  void withAudioGuard(const std::function<void()>& fn);

  IGfx& gfx_;
  MiniAcid& mini_acid_;
  AudioGuard& audio_guard_;
  int drum_step_cursor_;
  int drum_voice_cursor_;
  int drum_pattern_cursor_;
  int bank_index_;
  int bank_cursor_;
  bool bank_focus_;
  bool drum_pattern_focus_;
  std::shared_ptr<Component> grid_component_;
  std::shared_ptr<PatternSelectionBarComponent> pattern_bar_;
  std::shared_ptr<BankSelectionBarComponent> bank_bar_;
};
