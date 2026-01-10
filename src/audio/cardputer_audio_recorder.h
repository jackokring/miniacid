#pragma once

#include "audio_recorder.h"

#if defined(ARDUINO)
#include <SD.h>
#include <FS.h>

// Cardputer implementation that streams WAV to SD card
class CardputerAudioRecorder : public IAudioRecorder {
 public:
  CardputerAudioRecorder();
  ~CardputerAudioRecorder() override;

  bool start(int sampleRate, int channels) override;
  void stop() override;
  bool isRecording() const override;
  void writeSamples(const int16_t* samples, size_t sampleCount) override;
  const std::string& filename() const override;

 private:
  std::string generateTimestampFilename() const;
  void writeHeaderPlaceholder();
  void finalizeHeader();

  File file_;
  std::string filename_;
  std::uint32_t dataBytes_ = 0;
  int sampleRate_ = 0;
  int channels_ = 0;
};

#endif // ARDUINO
