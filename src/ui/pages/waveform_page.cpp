#include "waveform_page.h"

#include "../help_dialog.h"

namespace {
constexpr IGfxColor kWaveFadeColors[] = {
  IGfxColor(0x808080),
  IGfxColor(0x404040),
  IGfxColor(0x202020),
};
constexpr int kWaveFadeColorCount =
    static_cast<int>(sizeof(kWaveFadeColors) / sizeof(kWaveFadeColors[0]));
} // namespace

WaveformPage::WaveformPage(IGfx& gfx, MiniAcid& mini_acid, AudioGuard& audio_guard)
  : gfx_(gfx),
    mini_acid_(mini_acid),
    audio_guard_(audio_guard),
    wave_color_index_(0)
{
  for (int i = 0; i < kWaveHistoryLayers; ++i) {
    wave_lengths_[i] = 0;
  }
}

void WaveformPage::draw(IGfx& gfx, int x, int y, int w, int h) {
  int wave_y = y + 2;
  int wave_h = h - 2;
  if (w < 4 || wave_h < 4) return;

  int16_t samples[AUDIO_BUFFER_SAMPLES/2];
  size_t sampleCount = mini_acid_.copyLastAudio(samples, AUDIO_BUFFER_SAMPLES/2);
  int mid_y = wave_y + wave_h / 2;

  gfx_.setTextColor(IGfxColor::Orange());
  gfx_.drawLine(x, mid_y, x + w - 1, mid_y);

  int points = w;
  if (points > kMaxWavePoints) points = kMaxWavePoints;
  if (sampleCount > 1 && points > 1) {
    int16_t new_wave[kMaxWavePoints];
    for (int px = 0; px < points; ++px) {
      size_t idx = static_cast<size_t>((uint64_t)px * (sampleCount - 1) / (points - 1));
      new_wave[px] = samples[idx];
    }

    for (int layer = kWaveHistoryLayers - 1; layer > 0; --layer) {
      wave_lengths_[layer] = wave_lengths_[layer - 1];
      for (int px = 0; px < wave_lengths_[layer]; ++px) {
        wave_history_[layer][px] = wave_history_[layer - 1][px];
      }
    }

    wave_lengths_[0] = points;
    for (int px = 0; px < points; ++px) {
      wave_history_[0][px] = new_wave[px];
    }
  }

  auto drawWave = [&](const int16_t* wave, int count, IGfxColor color) {
    int drawCount = count;
    if (drawCount > w) drawCount = w;
    if (drawCount < 2) return;
    int amplitude = wave_h / 2 - 2;
    if (amplitude < 1) amplitude = 1;
    for (int px = 0; px < drawCount - 1; ++px) {
      float s0 = wave[px] / 32768.0f;
      float s1 = wave[px + 1] / 32768.0f;
      int y0 = mid_y - static_cast<int>(s0 * amplitude);
      int y1 = mid_y - static_cast<int>(s1 * amplitude);
      drawLineColored(gfx_, x + px, y0, x + px + 1, y1, color);
    }
  };

  for (int layer = kWaveHistoryLayers - 1; layer >= 1; --layer) {
    int colorIndex = layer - 1;
    if (colorIndex >= kWaveFadeColorCount) colorIndex = kWaveFadeColorCount - 1;
    drawWave(wave_history_[layer], wave_lengths_[layer], kWaveFadeColors[colorIndex]);
  }

  IGfxColor waveColor = WAVE_COLORS[wave_color_index_ % NUM_WAVE_COLORS];
  drawWave(wave_history_[0], wave_lengths_[0], waveColor);
}

bool WaveformPage::handleEvent(UIEvent& ui_event) {
  if (ui_event.event_type != MINIACID_KEY_DOWN) return false;
  switch (ui_event.scancode) {
    case MINIACID_UP:
    case MINIACID_DOWN:
      wave_color_index_ = (wave_color_index_ + 1) % NUM_WAVE_COLORS;
      return true;
    default:
      break;
  }
  return false;
}

const std::string & WaveformPage::getTitle() const {
  static std::string title = "WAVEFORM";
  return title;
}

void WaveformPage::drawHelpBody(IGfx& gfx, int x, int y, int w, int h) {
  (void)gfx;
  (void)x;
  (void)y;
  (void)w;
  (void)h;
}

bool WaveformPage::handleHelpEvent(UIEvent& ui_event) {
  (void)ui_event;
  return false;
}

bool WaveformPage::hasHelpDialog() {
  return false;
}
