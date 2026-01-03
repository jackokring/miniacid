#pragma once

#include "../ui_core.h"
#include "../ui_colors.h"
#include "../ui_utils.h"

class HelpPage : public IPage {
 public:
  explicit HelpPage();
  void draw(IGfx& gfx, int x, int y, int w, int h) override;
  void drawHelpBody(IGfx& gfx, int x, int y, int w, int h) override;
  bool handleEvent(UIEvent& ui_event) override;
  bool handleHelpEvent(UIEvent& ui_event) override;
  const std::string & getTitle() const override;
  bool hasHelpDialog() override;

 private:
  void drawTransportSection(IGfx& gfx, int x, int y, int w, int h);
};
