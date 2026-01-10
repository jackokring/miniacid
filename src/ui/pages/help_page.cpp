#include "help_page.h"

#include "../help_dialog_frames.h"

HelpPage::HelpPage() = default;

void HelpPage::drawTransportSection(IGfx& gfx, int x, int y, int w, int h) {
  drawHelpPageTransport(gfx, x, y, w, h);
}

void HelpPage::draw(IGfx& gfx) {
  const Rect& bounds = getBoundaries();
  drawTransportSection(gfx, bounds.x, bounds.y, bounds.w, bounds.h);
}

bool HelpPage::handleEvent(UIEvent& ui_event) {
  (void)ui_event;
  return false;
}

const std::string & HelpPage::getTitle() const {
  static std::string title = "HELP";
  return title;
}
