#include "ui_core.h"

#include "pages/help_dialog.h"

std::unique_ptr<MultiPageHelpDialog> IPage::getHelpDialog() {
  return nullptr;
}
