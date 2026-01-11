#include "mini_drumvoices.h"

#include <math.h>
#include <stdlib.h>

DrumSynthVoice::DrumSynthVoice(float sampleRate)
  : sampleRate(sampleRate),
    invSampleRate(0.0f) {
  setSampleRate(sampleRate);
  reset();
}

void DrumSynthVoice::reset() {
  kickPhase = 0.0f;
  kickFreq = 60.0f;
  kickEnvAmp = 0.0f;
  kickEnvPitch = 0.0f;
  kickActive = false;
  kickAccentGain = 1.0f;
  kickAccentDistortion = false;
  kickAmpDecay = 0.9995f;
  kickBaseFreq = 42.0f;

  snareEnvAmp = 0.0f;
  snareToneEnv = 0.0f;
  snareActive = false;
  snareBp = 0.0f;
  snareLp = 0.0f;
  snareTonePhase = 0.0f;
  snareTonePhase2 = 0.0f;
  snareAccentGain = 1.0f;
  snareToneGain = 1.0f;
  snareAccentDistortion = false;

  hatEnvAmp = 0.0f;
  hatToneEnv = 0.0f;
  hatActive = false;
  hatHp = 0.0f;
  hatPrev = 0.0f;
  hatPhaseA = 0.0f;
  hatPhaseB = 0.0f;
  hatAccentGain = 1.0f;
  hatBrightness = 1.0f;
  hatAccentDistortion = false;

  openHatEnvAmp = 0.0f;
  openHatToneEnv = 0.0f;
  openHatActive = false;
  openHatHp = 0.0f;
  openHatPrev = 0.0f;
  openHatPhaseA = 0.0f;
  openHatPhaseB = 0.0f;
  openHatAccentGain = 1.0f;
  openHatBrightness = 1.0f;
  openHatAccentDistortion = false;

  midTomPhase = 0.0f;
  midTomEnv = 0.0f;
  midTomActive = false;
  midTomAccentGain = 1.0f;
  midTomAccentDistortion = false;

  highTomPhase = 0.0f;
  highTomEnv = 0.0f;
  highTomActive = false;
  highTomAccentGain = 1.0f;
  highTomAccentDistortion = false;

  rimPhase = 0.0f;
  rimEnv = 0.0f;
  rimActive = false;
  rimAccentGain = 1.0f;
  rimAccentDistortion = false;

  clapEnv = 0.0f;
  clapTrans = 0.0f;
  clapNoise = 0.0f;
  clapActive = false;
  clapDelay = 0.0f;
  clapAccentGain = 1.0f;
  clapAccentDistortion = false;

  accentDistortion.setEnabled(true);
  accentDistortion.setDrive(3.0f);

  params[static_cast<int>(DrumParamId::MainVolume)] = Parameter("vol", "", 0.0f, 1.0f, 0.8f, 1.0f / 128);
}

void DrumSynthVoice::setSampleRate(float sampleRateHz) {
  if (sampleRateHz <= 0.0f) sampleRateHz = 44100.0f;
  sampleRate = sampleRateHz;
  invSampleRate = 1.0f / sampleRate;
}

void DrumSynthVoice::triggerKick(bool accent) {
  kickActive = true;
  kickPhase = 0.0f;
  kickEnvAmp = accent ? 1.4f : 1.2f;
  kickEnvPitch = 1.0f;
  kickFreq = 55.0f;
  kickAccentGain = accent ? 1.15f : 1.0f;
  kickAccentDistortion = accent;
  kickAmpDecay = accent ? 0.99965f : 0.9995f;
  kickBaseFreq = accent ? 36.0f : 42.0f;
}

void DrumSynthVoice::triggerSnare(bool accent) {
  snareActive = true;
  snareEnvAmp = accent ? 1.4f : 1.0f;
  snareToneEnv = accent ? 1.35f : 1.0f;
  snareTonePhase = 0.0f;
  snareTonePhase2 = 0.0f;
  snareAccentGain = accent ? 1.15f : 1.0f;
  snareToneGain = accent ? 1.2f : 1.0f;
  snareAccentDistortion = accent;
}

void DrumSynthVoice::triggerHat(bool accent) {
  hatActive = true;
  hatEnvAmp = accent ? 0.7f : 0.5f;
  hatToneEnv = 1.0f;
  hatPhaseA = 0.0f;
  hatPhaseB = 0.25f;
  hatAccentGain = accent ? 1.4f : 1.0f;
  hatBrightness = accent ? 1.45f : 1.0f;
  hatAccentDistortion = accent;
  // closing the hat chokes any ringing open-hat tail
  openHatEnvAmp *= 0.3f;
}

void DrumSynthVoice::triggerOpenHat(bool accent) {
  openHatActive = true;
  openHatEnvAmp = accent ? 0.999f : 0.9f;
  openHatToneEnv = 1.0f;
  openHatPhaseA = 0.0f;
  openHatPhaseB = 0.37f;
  openHatAccentGain = accent ? 1.3f : 1.0f;
  openHatBrightness = accent ? 1.25f : 1.0f;
  openHatAccentDistortion = accent;
}

void DrumSynthVoice::triggerMidTom(bool accent) {
  midTomActive = true;
  midTomEnv = 1.0f;
  midTomPhase = 0.0f;
  midTomAccentGain = accent ? 1.45f : 1.0f;
  midTomAccentDistortion = accent;
}

void DrumSynthVoice::triggerHighTom(bool accent) {
  highTomActive = true;
  highTomEnv = 1.0f;
  highTomPhase = 0.0f;
  highTomAccentGain = accent ? 1.45f : 1.0f;
  highTomAccentDistortion = accent;
}

void DrumSynthVoice::triggerRim(bool accent) {
  rimActive = true;
  rimEnv = 1.0f;
  rimPhase = 0.0f;
  rimAccentGain = accent ? 1.4f : 1.0f;
  rimAccentDistortion = accent;
}

void DrumSynthVoice::triggerClap(bool accent) {
  clapActive = true;
  clapEnv = 1.0f;
  clapTrans = 1.0f;
  clapNoise = frand();
  clapDelay = 0.0f;
  clapAccentGain = accent ? 1.45f : 1.0f;
  clapAccentDistortion = accent;
}

float DrumSynthVoice::frand() {
  return (float)rand() / (float)RAND_MAX * 2.0f - 1.0f;
}

float DrumSynthVoice::applyAccentDistortion(float input, bool accent) {
  if (!accent) {
    return input;
  }
  return accentDistortion.process(input);
}

float DrumSynthVoice::processKick() {
  if (!kickActive)
    return 0.0f;

  // Longer amp tail with faster pitch drop for a punchy thump
  kickEnvAmp *= kickAmpDecay;
  kickEnvPitch *= 0.997f;
  if (kickEnvAmp < 0.0008f) {
    kickActive = false;
    return 0.0f;
  }

  float pitchFactor = kickEnvPitch * kickEnvPitch;
  float f = kickBaseFreq + 170.0f * pitchFactor;
  kickFreq = f;
  kickPhase += kickFreq * invSampleRate;
  if (kickPhase >= 1.0f)
    kickPhase -= 1.0f;

  float body = sinf(2.0f * 3.14159265f * kickPhase);
  float transient = sinf(2.0f * 3.14159265f * kickPhase * 3.0f) * pitchFactor * 0.25f;
  float driven = tanhf(body * (2.8f + 0.6f * kickEnvAmp));

  float out = (driven * 0.85f + transient) * kickEnvAmp * kickAccentGain;
  return applyAccentDistortion(out, kickAccentDistortion);
}


float DrumSynthVoice::processSnare() {
  if (!snareActive)
    return 0.0f;

  // --- ENVELOPES ---
  // 808: Long noise decay, short tone decay
  snareEnvAmp *= 0.9985f;   // slow decay, long tail
  snareToneEnv *= 0.99999f;    // short tone "tick"

  if (snareEnvAmp < 0.0002f) {
    snareActive = false;
    return 0.0f;
  }

  // --- NOISE PROCESSING ---
  float n = frand(); // assume 0.0–1.0 random

  // 808: Noise is brighter with a bit of highpass emphasis
  // simple bandpass around ~1–2 kHz
  float f = 0.28f;
  snareBp += f * (n - snareLp - 0.20f * snareBp);
  snareLp += f * snareBp;

  // high fizz (808 has a lot of it)
  float noiseHP = n - snareLp;    // crude highpass
  float noiseOut = snareBp * 0.35f + noiseHP * 0.65f;

  // --- TONE (two sines, tuned to classic 808) ---
  // ~330 Hz + ~180 Hz slight mix, short decay
  snareTonePhase += 330.0f * invSampleRate;
  if (snareTonePhase >= 1.0f) snareTonePhase -= 1.0f;
  snareTonePhase2 += 180.0f * invSampleRate;
  if (snareTonePhase2 >= 1.0f) snareTonePhase2 -= 1.0f;

  float toneA = sinf(2.0f * 3.14159265f * snareTonePhase);
  float toneB = sinf(2.0f * 3.14159265f * snareTonePhase2);
  float tone = (toneA * 0.55f + toneB * 0.45f) * snareToneEnv * snareToneGain;

  // --- MIX ---
  // 808: tone only supports transient, noise dominates sustain
  float out = noiseOut * 0.75f + tone * 0.65f;
  out *= snareEnvAmp * snareAccentGain;
  return applyAccentDistortion(out, snareAccentDistortion);
}

float DrumSynthVoice::processHat() {
  if (!hatActive)
    return 0.0f;

  // hatEnvAmp *= 0.994f;   // slower decay for a longer hat tail
  hatEnvAmp *= 0.998f;   // slower decay for a longer hat tail
  hatToneEnv *= 0.92f;
  if (hatEnvAmp < 0.0005f) {
    hatActive = false;
    return 0.0f;
  }

  float n = frand();
  // crude highpass
  float alpha = 0.92f;
  hatHp = alpha * (hatHp + n - hatPrev);
  hatPrev = n;

  // Simple metallic partials on top of noise
  hatPhaseA += 6200.0f * invSampleRate;
  if (hatPhaseA >= 1.0f)
    hatPhaseA -= 1.0f;
  hatPhaseB += 7400.0f * invSampleRate;
  if (hatPhaseB >= 1.0f)
    hatPhaseB -= 1.0f;
  float tone = (sinf(2.0f * 3.14159265f * hatPhaseA) + sinf(2.0f * 3.14159265f * hatPhaseB)) *
               0.5f * hatToneEnv * hatBrightness;

  float out = hatHp * 0.65f + tone * 0.7f;
  out *= hatEnvAmp * 0.6f * hatAccentGain;
  return applyAccentDistortion(out, hatAccentDistortion);
}

float DrumSynthVoice::processOpenHat() {
  if (!openHatActive)
    return 0.0f;

  openHatEnvAmp *= 0.9993f;
  openHatToneEnv *= 0.94f;
  if (openHatEnvAmp < 0.0004f) {
    openHatActive = false;
    return 0.0f;
  }

  float n = frand();
  float alpha = 0.93f;
  openHatHp = alpha * (openHatHp + n - openHatPrev);
  openHatPrev = n;

  openHatPhaseA += 5100.0f * invSampleRate;
  if (openHatPhaseA >= 1.0f)
    openHatPhaseA -= 1.0f;
  openHatPhaseB += 6600.0f * invSampleRate;
  if (openHatPhaseB >= 1.0f)
    openHatPhaseB -= 1.0f;
  float tone =
    (sinf(2.0f * 3.14159265f * openHatPhaseA) + sinf(2.0f * 3.14159265f * openHatPhaseB)) *
    0.5f * openHatToneEnv * openHatBrightness;

  float out = openHatHp * 0.55f + tone * 0.95f;
  out *= openHatEnvAmp * 0.7f * openHatAccentGain;
  return applyAccentDistortion(out, openHatAccentDistortion);
}

float DrumSynthVoice::processMidTom() {
  if (!midTomActive)
    return 0.0f;

  midTomEnv *= 0.99925f;
  if (midTomEnv < 0.0003f) {
    midTomActive = false;
    return 0.0f;
  }

  float freq = 180.0f;
  midTomPhase += freq * invSampleRate;
  if (midTomPhase >= 1.0f)
    midTomPhase -= 1.0f;

  float tone = sinf(2.0f * 3.14159265f * midTomPhase);
  float slightNoise = frand() * 0.05f;
  float out = (tone * 0.9f + slightNoise) * midTomEnv * 0.8f * midTomAccentGain;
  return applyAccentDistortion(out, midTomAccentDistortion);
}

float DrumSynthVoice::processHighTom() {
  if (!highTomActive)
    return 0.0f;

  highTomEnv *= 0.99915f;
  if (highTomEnv < 0.0003f) {
    highTomActive = false;
    return 0.0f;
  }

  float freq = 240.0f;

  highTomPhase += freq * invSampleRate;
  if (highTomPhase >= 1.0f)
    highTomPhase -= 1.0f;

  float tone = sinf(2.0f * 3.14159265f * highTomPhase);
  float slightNoise = frand() * 0.04f;
  float out = (tone * 0.88f + slightNoise) * highTomEnv * 0.75f * highTomAccentGain;
  return applyAccentDistortion(out, highTomAccentDistortion);
}

float DrumSynthVoice::processRim() {
  if (!rimActive)
    return 0.0f;

  rimEnv *= 0.9985f;
  if (rimEnv < 0.0004f) {
    rimActive = false;
    return 0.0f;
  }

  rimPhase += 900.0f * invSampleRate;
  if (rimPhase >= 1.0f)
    rimPhase -= 1.0f;
  float tone = sinf(2.0f * 3.14159265f * rimPhase);
  float click = (frand() * 0.6f + 0.4f) * rimEnv;
  float out = (tone * 0.5f + click) * rimEnv * 0.8f * rimAccentGain;
  return applyAccentDistortion(out, rimAccentDistortion);
}

float DrumSynthVoice::processClap() {
  if (!clapActive)
    return 0.0f;

  clapEnv *= 0.99992f;
  clapTrans *= 0.9985f;
  clapDelay += invSampleRate;
  if (clapEnv < 0.0002f) {
    clapActive = false;
    return 0.0f;
  }

  // simple three-burst clap feel
  float burst = 0.0f;
  if (clapDelay < 0.024f) burst = 1.0f;
  else if (clapDelay < 0.048f) burst = 0.8f;
  else if (clapDelay < 0.072f) burst = 0.6f;

  float noise = frand() * 0.7f + clapNoise * 0.3f;
  float tone = sinf(2.0f * 3.14159265f * 1100.0f * clapDelay);
  float out = (noise * 0.7f + tone * 0.3f) * clapTrans * burst;
  out *= clapEnv * clapAccentGain;
  return applyAccentDistortion(out, clapAccentDistortion);
}

const Parameter& DrumSynthVoice::parameter(DrumParamId id) const {
  return params[static_cast<int>(id)];
}

void DrumSynthVoice::setParameter(DrumParamId id, float value) {
  params[static_cast<int>(id)].setValue(value);
}
