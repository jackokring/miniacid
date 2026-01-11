#pragma once

#include <stdint.h>

#include "mini_dsp_params.h"
#include "tube_distortion.h"

enum class DrumParamId : uint8_t {
  MainVolume = 0,
  Count
};

class DrumSynthVoice {
public:
  explicit DrumSynthVoice(float sampleRate);

  void reset();
  void setSampleRate(float sampleRate);
  void triggerKick(bool accent = false);
  void triggerSnare(bool accent = false);
  void triggerHat(bool accent = false);
  void triggerOpenHat(bool accent = false);
  void triggerMidTom(bool accent = false);
  void triggerHighTom(bool accent = false);
  void triggerRim(bool accent = false);
  void triggerClap(bool accent = false);

  float processKick();
  float processSnare();
  float processHat();
  float processOpenHat();
  float processMidTom();
  float processHighTom();
  float processRim();
  float processClap();

  const Parameter& parameter(DrumParamId id) const;
  void setParameter(DrumParamId id, float value);

private:
  float frand();
  float applyAccentDistortion(float input, bool accent);

  float kickPhase;
  float kickFreq;
  float kickEnvAmp;
  float kickEnvPitch;
  bool kickActive;
  float kickAccentGain;
  bool kickAccentDistortion;
  float kickAmpDecay;
  float kickBaseFreq;

  float snareEnvAmp;
  float snareToneEnv;
  bool snareActive;
  float snareBp;
  float snareLp;
  float snareTonePhase;
  float snareTonePhase2;
  float snareAccentGain;
  float snareToneGain;
  bool snareAccentDistortion;

  float hatEnvAmp;
  float hatToneEnv;
  bool hatActive;
  float hatHp;
  float hatPrev;
  float hatPhaseA;
  float hatPhaseB;
  float hatAccentGain;
  float hatBrightness;
  bool hatAccentDistortion;

  float openHatEnvAmp;
  float openHatToneEnv;
  bool openHatActive;
  float openHatHp;
  float openHatPrev;
  float openHatPhaseA;
  float openHatPhaseB;
  float openHatAccentGain;
  float openHatBrightness;
  bool openHatAccentDistortion;

  float midTomPhase;
  float midTomEnv;
  bool midTomActive;
  float midTomAccentGain;
  bool midTomAccentDistortion;

  float highTomPhase;
  float highTomEnv;
  bool highTomActive;
  float highTomAccentGain;
  bool highTomAccentDistortion;

  float rimPhase;
  float rimEnv;
  bool rimActive;
  float rimAccentGain;
  bool rimAccentDistortion;

  float clapEnv;
  float clapTrans;
  float clapNoise;
  bool clapActive;
  float clapDelay;
  float clapAccentGain;
  bool clapAccentDistortion;

  float sampleRate;
  float invSampleRate;

  TubeDistortion accentDistortion;

  Parameter params[static_cast<int>(DrumParamId::Count)];
};
