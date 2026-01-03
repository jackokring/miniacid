#include "help_page.h"

#include "../help_dialog.h"

HelpPage::HelpPage() = default;

void HelpPage::drawTransportSection(IGfx& gfx, int x, int y, int w, int h) {
  drawHelpPageTransport(gfx, x, y, w, h);
}

void HelpPage::draw(IGfx& gfx, int x, int y, int w, int h) {
  drawTransportSection(gfx, x, y, w, h);
}

void HelpPage::drawHelpBody(IGfx& gfx, int x, int y, int w, int h) {
  drawTransportSection(gfx, x, y, w, h);
}

bool HelpPage::handleEvent(UIEvent& ui_event) {
  (void)ui_event;
  return false;
}

bool HelpPage::handleHelpEvent(UIEvent& ui_event) {
  (void)ui_event;
  return false;
}

const std::string & HelpPage::getTitle() const {
  static std::string title = "HELP";
  return title;
}

bool HelpPage::hasHelpDialog() {
  return false;
}
