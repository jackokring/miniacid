#pragma once


#include "audio_recorder.h"
#include <cstdio>

// Desktop implementation using FILE* to write WAV files
class DesktopAudioRecorder : public IAudioRecorder {
 public:
  DesktopAudioRecorder();
  ~DesktopAudioRecorder() override;

  bool start(int sampleRate, int channels) override;
  void stop() override;
  bool isRecording() const override;
  void writeSamples(const int16_t* samples, size_t sampleCount) override;
  const std::string& filename() const override;

 private:
  std::string generateTimestampFilename() const;
  void writeHeaderPlaceholder();
  void finalizeHeader();

  std::FILE* file_ = nullptr;
  std::string filename_;
  std::uint32_t dataBytes_ = 0;
  int sampleRate_ = 0;
  int channels_ = 0;
};
