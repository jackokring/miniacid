#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

// Abstract interface for audio recording
class IAudioRecorder {
 public:
  virtual ~IAudioRecorder() = default;

  virtual bool start(int sampleRate, int channels) = 0;
  virtual void stop() = 0;
  virtual bool isRecording() const = 0;
  virtual void writeSamples(const int16_t* samples, size_t sampleCount) = 0;
  virtual const std::string& filename() const = 0;
};
