#pragma once

#include "audio_recorder.h"

#if defined(__EMSCRIPTEN__)
#include <vector>

// WASM implementation that accumulates data and calls JS callback on stop
class WasmAudioRecorder : public IAudioRecorder {
 public:
  WasmAudioRecorder();
  ~WasmAudioRecorder() override;

  bool start(int sampleRate, int channels) override;
  void stop() override;
  bool isRecording() const override;
  void writeSamples(const int16_t* samples, size_t sampleCount) override;
  const std::string& filename() const override;

 private:
  std::string generateTimestampFilename() const;
  void sendToJavaScript();

  std::vector<int16_t> buffer_;
  std::string filename_;
  int sampleRate_ = 0;
  int channels_ = 0;
  bool recording_ = false;
};

#endif // __EMSCRIPTEN__
