#include "dsp_engine.h"

#include <math.h>
#include <stdlib.h>
#include <algorithm>

Parameter::Parameter()
  : _label(""), _unit(""), _min(0.0f), _max(1.0f),
    _default(0.0f), _step(0.0f), _value(0.0f) {}

Parameter::Parameter(const char* label, const char* unit, float minValue, float maxValue, float defaultValue, float step)
  : _label(label), _unit(unit), _min(minValue), _max(maxValue),
    _default(defaultValue), _step(step), _value(defaultValue) {}

const char* Parameter::label() const { return _label; }
const char* Parameter::unit() const { return _unit; }
float Parameter::value() const { return _value; }
float Parameter::min() const { return _min; }
float Parameter::max() const { return _max; }
float Parameter::step() const { return _step; }

float Parameter::normalized() const {
  if (_max <= _min) return 0.0f;
  return (_value - _min) / (_max - _min);
}

void Parameter::setValue(float v) {
  if (v < _min) v = _min;
  if (v > _max) v = _max;
  _value = v;
}

void Parameter::addSteps(int steps) {
  setValue(_value + _step * steps);
}

void Parameter::setNormalized(float norm) {
  if (norm < 0.0f) norm = 0.0f;
  if (norm > 1.0f) norm = 1.0f;
  _value = _min + norm * (_max - _min);
}

void Parameter::reset() {
  _value = _default;
}

ChamberlinFilter::ChamberlinFilter(float sampleRate) : _lp(0.0f), _bp(0.0f), _sampleRate(sampleRate) {
  if (_sampleRate <= 0.0f) _sampleRate = 44100.0f;
}

void ChamberlinFilter::reset() {
  _lp = 0.0f;
  _bp = 0.0f;
}

void ChamberlinFilter::setSampleRate(float sr) {
  if (sr <= 0.0f) sr = 44100.0f;
  _sampleRate = sr;
}

float ChamberlinFilter::process(float input, float cutoffHz, float resonance) {
  float f = 2.0f * sinf(3.14159265f * cutoffHz / _sampleRate);
  if (!isfinite(f))
    f = 0.0f;
  float q = 1.0f / (1.0f + resonance * 4.0f);
  if (q < 0.06f)
    q = 0.06f;
  

  float hp = input - _lp - q * _bp;
  _bp += f * hp;
  _lp += f * _bp;

  _bp = tanhf(_bp * 1.3f);

  // Keep states bounded to avoid numeric blowups
  const float kStateLimit = 50.0f;
  if (_lp > kStateLimit) _lp = kStateLimit;
  if (_lp < -kStateLimit) _lp = -kStateLimit;
  if (_bp > kStateLimit) _bp = kStateLimit;
  if (_bp < -kStateLimit) _bp = -kStateLimit;

  return _lp;
}

TB303Voice::TB303Voice(float sampleRate)
  : sampleRate(sampleRate),
    invSampleRate(0.0f),
    nyquist(0.0f),
    filter(sampleRate) {
  setSampleRate(sampleRate);
  reset();
}

void TB303Voice::reset() {
  initParameters();
  phase = 0.0f;
  freq = 110.0f;
  targetFreq = 110.0f;
  slideSpeed = 0.002f;
  env = 0.0f;
  gate = false;
  slide = false;
  amp = 0.3f;
  filter.reset();
}

void TB303Voice::setSampleRate(float sampleRateHz) {
  if (sampleRateHz <= 0.0f) sampleRateHz = 44100.0f;
  sampleRate = sampleRateHz;
  invSampleRate = 1.0f / sampleRate;
  nyquist = sampleRate * 0.5f;
  filter.setSampleRate(sampleRate);
}

void TB303Voice::startNote(float freqHz, bool accent, bool slideFlag) {
  slide = slideFlag;

  if (!slide) {
    freq = freqHz;
  }
  targetFreq = freqHz;

  gate = true;
  env = accent ? 1.5f : 1.0f;
}

void TB303Voice::release() { gate = false; }

float TB303Voice::oscSaw() {
  phase += freq * invSampleRate;
  if (phase >= 1.0f) {
    phase -= 1.0f;
  }
  return 2.0f * phase - 1.0f;
}

float TB303Voice::svfProcess(float input) {
  // Slide toward target frequency
  freq += (targetFreq - freq) * slideSpeed;
  if (!isfinite(freq))
    freq = targetFreq;

  // Envelope decay
  if (gate || env > 0.0001f) {
    float decayMs = parameterValue(TB303ParamId::EnvDecay);
    float decaySamples = decayMs * sampleRate * 0.001f;
    if (decaySamples < 1.0f)
      decaySamples = 1.0f;
    // 0.01 represents roughly -40 dB, a practical "off" point for the envelope.
    constexpr float kDecayTargetLog = -4.60517019f; // ln(0.01f)
    float decayCoeff = expf(kDecayTargetLog / decaySamples);
    env *= decayCoeff;
  }

  float cutoffHz = parameterValue(TB303ParamId::Cutoff) + parameterValue(TB303ParamId::EnvAmount) * env;
  if (cutoffHz < 50.0f)
    cutoffHz = 50.0f;
  float maxCutoff = nyquist * 0.9f;
  if (cutoffHz > maxCutoff)
    cutoffHz = maxCutoff;


  return filter.process(input, cutoffHz, parameterValue(TB303ParamId::Resonance));
}

float TB303Voice::process() {
  if (!gate && env < 0.0001f) {
    return 0.0f;
  }

  float osc = oscSaw();
  float out = svfProcess(osc);

  return out * amp;
}

const Parameter& TB303Voice::parameter(TB303ParamId id) const {
  return params[static_cast<int>(id)];
}

void TB303Voice::setParameter(TB303ParamId id, float value) {
  params[static_cast<int>(id)].setValue(value);
}

void TB303Voice::adjustParameter(TB303ParamId id, int steps) {
  params[static_cast<int>(id)].addSteps(steps);
}

float TB303Voice::parameterValue(TB303ParamId id) const {
  return params[static_cast<int>(id)].value();
}

void TB303Voice::initParameters() {
  params[static_cast<int>(TB303ParamId::Cutoff)] = Parameter("cut", "Hz", 60.0f, 2500.0f, 800.0f, 100.0f);
  params[static_cast<int>(TB303ParamId::Resonance)] = Parameter("res", "", 0.05f, 0.85f, 0.6f, 0.05f);
  params[static_cast<int>(TB303ParamId::EnvAmount)] = Parameter("env", "Hz", 0.0f, 2000.0f, 400.0f, 200.0f);
  params[static_cast<int>(TB303ParamId::EnvDecay)] = Parameter("dec", "ms", 20.0f, 2200.0f, 420.0f, 50.0f);
}

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

  snareEnvAmp = 0.0f;
  snareToneEnv = 0.0f;
  snareActive = false;
  snareBp = 0.0f;
  snareLp = 0.0f;
  snareTonePhase = 0.0f;
  snareTonePhase2 = 0.0f;

  hatEnvAmp = 0.0f;
  hatToneEnv = 0.0f;
  hatActive = false;
  hatHp = 0.0f;
  hatPrev = 0.0f;
  hatPhaseA = 0.0f;
  hatPhaseB = 0.0f;

  openHatEnvAmp = 0.0f;
  openHatToneEnv = 0.0f;
  openHatActive = false;
  openHatHp = 0.0f;
  openHatPrev = 0.0f;
  openHatPhaseA = 0.0f;
  openHatPhaseB = 0.0f;

  midTomPhase = 0.0f;
  midTomEnv = 0.0f;
  midTomActive = false;

  highTomPhase = 0.0f;
  highTomEnv = 0.0f;
  highTomActive = false;

  rimPhase = 0.0f;
  rimEnv = 0.0f;
  rimActive = false;

  clapEnv = 0.0f;
  clapTrans = 0.0f;
  clapNoise = 0.0f;
  clapActive = false;
  clapDelay = 0.0f;
}

void DrumSynthVoice::setSampleRate(float sampleRateHz) {
  if (sampleRateHz <= 0.0f) sampleRateHz = 44100.0f;
  sampleRate = sampleRateHz;
  invSampleRate = 1.0f / sampleRate;
}

void DrumSynthVoice::triggerKick() {
  kickActive = true;
  kickPhase = 0.0f;
  kickEnvAmp = 1.2f;
  kickEnvPitch = 1.0f;
  kickFreq = 55.0f;
}

void DrumSynthVoice::triggerSnare() {
  snareActive = true;
  snareEnvAmp = 1.1f;
  snareToneEnv = 1.0f;
  snareTonePhase = 0.0f;
  snareTonePhase2 = 0.0f;
}

void DrumSynthVoice::triggerHat() {
  hatActive = true;
  hatEnvAmp = 0.7f;
  hatToneEnv = 1.0f;
  hatPhaseA = 0.0f;
  hatPhaseB = 0.25f;
  // closing the hat chokes any ringing open-hat tail
  openHatEnvAmp *= 0.3f;
}

void DrumSynthVoice::triggerOpenHat() {
  openHatActive = true;
  openHatEnvAmp = 0.9f;
  openHatToneEnv = 1.0f;
  openHatPhaseA = 0.0f;
  openHatPhaseB = 0.37f;
}

void DrumSynthVoice::triggerMidTom() {
  midTomActive = true;
  midTomEnv = 1.0f;
  midTomPhase = 0.0f;
}

void DrumSynthVoice::triggerHighTom() {
  highTomActive = true;
  highTomEnv = 1.0f;
  highTomPhase = 0.0f;
}

void DrumSynthVoice::triggerRim() {
  rimActive = true;
  rimEnv = 1.0f;
  rimPhase = 0.0f;
}

void DrumSynthVoice::triggerClap() {
  clapActive = true;
  clapEnv = 1.0f;
  clapTrans = 1.0f;
  clapNoise = frand();
  clapDelay = 0.0f;
}

float DrumSynthVoice::frand() {
  return (float)rand() / (float)RAND_MAX * 2.0f - 1.0f;
}

float DrumSynthVoice::processKick() {
  if (!kickActive)
    return 0.0f;

  // Longer amp tail with faster pitch drop for a punchy thump
  kickEnvAmp *= 0.9995f;
  kickEnvPitch *= 0.997f;
  if (kickEnvAmp < 0.0008f) {
    kickActive = false;
    return 0.0f;
  }

  float pitchFactor = kickEnvPitch * kickEnvPitch;
  float f = 42.0f + 170.0f * pitchFactor;
  kickFreq = f;
  kickPhase += kickFreq * invSampleRate;
  if (kickPhase >= 1.0f)
    kickPhase -= 1.0f;

  float body = sinf(2.0f * 3.14159265f * kickPhase);
  float transient = sinf(2.0f * 3.14159265f * kickPhase * 3.0f) * pitchFactor * 0.25f;
  float driven = tanhf(body * (2.8f + 0.6f * kickEnvAmp));

  return (driven * 0.85f + transient) * kickEnvAmp;
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
  float tone = (toneA * 0.55f + toneB * 0.45f) * snareToneEnv;

  // --- MIX ---
  // 808: tone only supports transient, noise dominates sustain
  float out = noiseOut * 0.75f + tone * 0.65f;
  return out * snareEnvAmp;
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
  float tone = (sinf(2.0f * 3.14159265f * hatPhaseA) + sinf(2.0f * 3.14159265f * hatPhaseB)) * 0.5f * hatToneEnv;

  float out = hatHp * 0.65f + tone * 0.7f;
  return out * hatEnvAmp * 0.6f;
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
  float tone = (sinf(2.0f * 3.14159265f * openHatPhaseA) + sinf(2.0f * 3.14159265f * openHatPhaseB)) * 0.5f * openHatToneEnv;

  float out = openHatHp * 0.55f + tone * 0.95f;
  return out * openHatEnvAmp * 0.7f;
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
  return (tone * 0.9f + slightNoise) * midTomEnv * 0.8f;
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
  return (tone * 0.88f + slightNoise) * highTomEnv * 0.75f;
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
  return (tone * 0.5f + click) * rimEnv * 0.8f;
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
  return out * clapEnv;
}

TempoDelay::TempoDelay(float sampleRate)
  : buffer(),
    writeIndex(0),
    delaySamples(1),
    sampleRate(0.0f),
    maxDelaySamples(0),
    beats(0.25f),
    mix(0.35f),
    feedback(0.45f),
    enabled(false) {
  setSampleRate(sampleRate);
  reset();
}

void TempoDelay::reset() {
  if (buffer.empty())
    return;
  std::fill(buffer.begin(), buffer.end(), 0.0f);
  writeIndex = 0;
  if (delaySamples < 1)
    delaySamples = 1;
  if (delaySamples >= maxDelaySamples)
    delaySamples = maxDelaySamples - 1;
}

void TempoDelay::setSampleRate(float sr) {
  if (sr <= 0.0f) sr = 44100.0f;
  sampleRate = sr;
  maxDelaySamples = static_cast<int>(sampleRate * kMaxDelaySeconds);
  if (maxDelaySamples < 1)
    maxDelaySamples = 1;
  buffer.assign(static_cast<size_t>(maxDelaySamples), 0.0f);
  if (delaySamples >= maxDelaySamples)
    delaySamples = maxDelaySamples - 1;
  if (delaySamples < 1)
    delaySamples = 1;
}

void TempoDelay::setBpm(float bpm) {
  if (bpm < 40.0f)
    bpm = 40.0f;
  float secondsPerBeat = 60.0f / bpm;
  float delaySeconds = secondsPerBeat * beats;
  int samples = static_cast<int>(delaySeconds * sampleRate);
  if (samples < 1)
    samples = 1;
  if (samples >= maxDelaySamples)
    samples = maxDelaySamples - 1;
  delaySamples = samples;
}

void TempoDelay::setBeats(float b) {
  if (b < 0.125f)
    b = 0.125f;
  beats = b;
}

void TempoDelay::setMix(float m) {
  if (m < 0.0f)
    m = 0.0f;
  if (m > 1.0f)
    m = 1.0f;
  mix = m;
}

void TempoDelay::setFeedback(float fb) {
  if (fb < 0.0f)
    fb = 0.0f;
  if (fb > 0.95f)
    fb = 0.95f;
  feedback = fb;
}

void TempoDelay::setEnabled(bool on) { enabled = on; }

bool TempoDelay::isEnabled() const { return enabled; }

float TempoDelay::process(float input) {
  if (!enabled || buffer.empty()) {
    return input;
  }

  int readIndex = writeIndex - delaySamples;
  if (readIndex < 0)
    readIndex += maxDelaySamples;

  float delayed = buffer[readIndex];
  buffer[writeIndex] = input + delayed * feedback;

  writeIndex++;
  if (writeIndex >= maxDelaySamples)
    writeIndex = 0;

  return input + delayed * mix;
}

MiniAcid::MiniAcid(float sampleRate)
  : voice303(sampleRate),
    voice3032(sampleRate),
    drums(sampleRate),
    sampleRateValue(sampleRate),
    playing(false),
    mute303(false),
    mute303_2(false),
    muteKick(false),
    muteSnare(false),
    muteHat(false),
    muteOpenHat(false),
    muteMidTom(false),
    muteHighTom(false),
    muteRim(false),
    muteClap(false),
    delay303Enabled(false),
    delay3032Enabled(false),
    bpmValue(100.0f),
    currentStepIndex(-1),
    samplesIntoStep(0),
    samplesPerStep(0.0f),
    delay303(sampleRate),
    delay3032(sampleRate) {
  if (sampleRateValue <= 0.0f) sampleRateValue = 44100.0f;

  // Default patterns
  int8_t notes[SEQ_STEPS] = {48, 48, 55, 55, 50, 50, 55, 55,
                             48, 48, 55, 55, 50, 55, 50, -1};
  bool accent[SEQ_STEPS] = {false, true, false, true, false, true,
                            false, true, false, true, false, true,
                            false, true, false, false};
  bool slide[SEQ_STEPS] = {false, true, false, true, false, true,
                           false, true, false, true, false, true,
                           false, true, false, false};

  bool kick[SEQ_STEPS] = {true,  false, false, false, true,  false,
                          false, false, true,  false, false, false,
                          true,  false, false, false};

  bool snare[SEQ_STEPS] = {false, false, true,  false, false, false,
                           true,  false, false, false, true,  false,
                           false, false, true,  false};

  bool hat[SEQ_STEPS] = {true, true, true, true, true, true, true, true,
                         true, true, true, true, true, true, true, true};

  bool openHat[SEQ_STEPS] = {false, false, false, true,  false, false, false, false,
                             false, false, false, true,  false, false, false, false};

  bool midTom[SEQ_STEPS] = {false, false, false, false, true,  false, false, false,
                            false, false, false, false, true,  false, false, false};

  bool highTom[SEQ_STEPS] = {false, false, false, false, false, false, true,  false,
                             false, false, false, false, false, false, true,  false};

  bool rim[SEQ_STEPS] = {false, false, false, false, false, true,  false, false,
                         false, false, false, false, false, true,  false, false};

  bool clap[SEQ_STEPS] = {false, false, false, false, false, false, false, false,
                          false, false, false, false, true,  false, false, false};

  int8_t notes2[SEQ_STEPS] = {48, 48, 55, 55, 50, 50, 55, 55,
                             48, 48, 55, 55, 50, 55, 50, -1};
  bool accent2[SEQ_STEPS] = {true,  false, true,  false, true,  false,
                             true,  false, true,  false, true,  false,
                             true,  false, true,  false};
  bool slide2[SEQ_STEPS] = {false, false, true,  false, false, false,
                            true,  false, false, false, true,  false,
                            false, false, true,  false};

  for (int i = 0; i < SEQ_STEPS; ++i) {
    pattern303[i] = notes[i];
    pattern303Accent[i] = accent[i];
    pattern303Slide[i] = slide[i];
    pattern303_2[i] = notes2[i];
    pattern303Accent2[i] = accent2[i];
    pattern303Slide2[i] = slide2[i];
    patternKick[i] = kick[i];
    patternSnare[i] = snare[i];
    patternHat[i] = hat[i];
    patternOpenHat[i] = openHat[i];
    if (patternOpenHat[i])
      patternHat[i] = false;
    patternMidTom[i] = midTom[i];
    patternHighTom[i] = highTom[i];
    patternRim[i] = rim[i];
    patternClap[i] = clap[i];
  }

  reset();
}

void MiniAcid::reset() {
  voice303.reset();
  voice3032.reset();
  // make the second voice have different params
  voice3032.adjustParameter(TB303ParamId::Cutoff, -3); 
  voice3032.adjustParameter(TB303ParamId::Resonance, -3); 
  voice3032.adjustParameter(TB303ParamId::EnvAmount, -1); 
  drums.reset();
  playing = false;
  mute303 = false;
  mute303_2 = false;
  muteKick = false;
  muteSnare = false;
  muteHat = false;
  muteOpenHat = false;
  muteMidTom = false;
  muteHighTom = false;
  muteRim = false;
  muteClap = false;
  delay303Enabled = false;
  delay3032Enabled = false;
  bpmValue = 110.0f;
  currentStepIndex = -1;
  samplesIntoStep = 0;
  updateSamplesPerStep();
  delay303.reset();
  delay303.setBeats(0.5f); // eighth note
  delay303.setMix(0.25f);
  delay303.setFeedback(0.35f);
  delay303.setEnabled(delay303Enabled);
  delay303.setBpm(bpmValue);
  delay3032.reset();
  delay3032.setBeats(0.5f);
  delay3032.setMix(0.22f);
  delay3032.setFeedback(0.32f);
  delay3032.setEnabled(delay3032Enabled);
  delay3032.setBpm(bpmValue);
  lastBufferCount = 0;
  for (int i = 0; i < AUDIO_BUFFER_SAMPLES; ++i) lastBuffer[i] = 0;
}

void MiniAcid::start() {
  playing = true;
  currentStepIndex = -1;
  samplesIntoStep = static_cast<unsigned long>(samplesPerStep);
}

void MiniAcid::stop() {
  playing = false;
  currentStepIndex = -1;
  samplesIntoStep = 0;
  voice303.release();
  voice3032.release();
  drums.reset();
}

void MiniAcid::setBpm(float bpm) {
  bpmValue = bpm;
  if (bpmValue < 40.0f)
    bpmValue = 40.0f;
  if (bpmValue > 200.0f)
    bpmValue = 200.0f;
  updateSamplesPerStep();
  delay303.setBpm(bpmValue);
  delay3032.setBpm(bpmValue);
}

float MiniAcid::bpm() const { return bpmValue; }
float MiniAcid::sampleRate() const { return sampleRateValue; }

bool MiniAcid::isPlaying() const { return playing; }

int MiniAcid::currentStep() const { return currentStepIndex; }

bool MiniAcid::is303Muted(int voiceIndex) const {
  int idx = clamp303Voice(voiceIndex);
  return idx == 0 ? mute303 : mute303_2;
}
bool MiniAcid::isKickMuted() const { return muteKick; }
bool MiniAcid::isSnareMuted() const { return muteSnare; }
bool MiniAcid::isHatMuted() const { return muteHat; }
bool MiniAcid::isOpenHatMuted() const { return muteOpenHat; }
bool MiniAcid::isMidTomMuted() const { return muteMidTom; }
bool MiniAcid::isHighTomMuted() const { return muteHighTom; }
bool MiniAcid::isRimMuted() const { return muteRim; }
bool MiniAcid::isClapMuted() const { return muteClap; }
bool MiniAcid::is303DelayEnabled(int voiceIndex) const {
  int idx = clamp303Voice(voiceIndex);
  return idx == 0 ? delay303Enabled : delay3032Enabled;
}
const Parameter& MiniAcid::parameter303(TB303ParamId id, int voiceIndex) const {
  int idx = clamp303Voice(voiceIndex);
  return idx == 0 ? voice303.parameter(id) : voice3032.parameter(id);
}
const int8_t* MiniAcid::pattern303Steps(int voiceIndex) const {
  int idx = clamp303Voice(voiceIndex);
  return idx == 0 ? pattern303 : pattern303_2;
}
const bool* MiniAcid::pattern303AccentSteps(int voiceIndex) const {
  int idx = clamp303Voice(voiceIndex);
  return idx == 0 ? pattern303Accent : pattern303Accent2;
}
const bool* MiniAcid::pattern303SlideSteps(int voiceIndex) const {
  int idx = clamp303Voice(voiceIndex);
  return idx == 0 ? pattern303Slide : pattern303Slide2;
}
const bool* MiniAcid::patternKickSteps() const { return patternKick; }
const bool* MiniAcid::patternSnareSteps() const { return patternSnare; }
const bool* MiniAcid::patternHatSteps() const { return patternHat; }
const bool* MiniAcid::patternOpenHatSteps() const { return patternOpenHat; }
const bool* MiniAcid::patternMidTomSteps() const { return patternMidTom; }
const bool* MiniAcid::patternHighTomSteps() const { return patternHighTom; }
const bool* MiniAcid::patternRimSteps() const { return patternRim; }
const bool* MiniAcid::patternClapSteps() const { return patternClap; }

size_t MiniAcid::copyLastAudio(int16_t *dst, size_t maxSamples) const {
  if (!dst || maxSamples == 0) return 0;
  size_t n = lastBufferCount;
  if (n > maxSamples) n = maxSamples;
  for (size_t i = 0; i < n; ++i) dst[i] = lastBuffer[i];
  return n;
}

void MiniAcid::toggleMute303(int voiceIndex) {
  int idx = clamp303Voice(voiceIndex);
  if (idx == 0)
    mute303 = !mute303;
  else
    mute303_2 = !mute303_2;
}
void MiniAcid::toggleMuteKick() { muteKick = !muteKick; }
void MiniAcid::toggleMuteSnare() { muteSnare = !muteSnare; }
void MiniAcid::toggleMuteHat() { muteHat = !muteHat; }
void MiniAcid::toggleMuteOpenHat() { muteOpenHat = !muteOpenHat; }
void MiniAcid::toggleMuteMidTom() { muteMidTom = !muteMidTom; }
void MiniAcid::toggleMuteHighTom() { muteHighTom = !muteHighTom; }
void MiniAcid::toggleMuteRim() { muteRim = !muteRim; }
void MiniAcid::toggleMuteClap() { muteClap = !muteClap; }
void MiniAcid::toggleDelay303(int voiceIndex) {
  int idx = clamp303Voice(voiceIndex);
  if (idx == 0) {
    delay303Enabled = !delay303Enabled;
    delay303.setEnabled(delay303Enabled);
  } else {
    delay3032Enabled = !delay3032Enabled;
    delay3032.setEnabled(delay3032Enabled);
  }
}
void MiniAcid::adjust303Parameter(TB303ParamId id, int steps, int voiceIndex) {
  int idx = clamp303Voice(voiceIndex);
  if (idx == 0)
    voice303.adjustParameter(id, steps);
  else
    voice3032.adjustParameter(id, steps);
}
void MiniAcid::set303Parameter(TB303ParamId id, float value, int voiceIndex) {
  int idx = clamp303Voice(voiceIndex);
  if (idx == 0)
    voice303.setParameter(id, value);
  else
    voice3032.setParameter(id, value);
}

int MiniAcid::clamp303Voice(int voiceIndex) const {
  if (voiceIndex < 0) return 0;
  if (voiceIndex >= NUM_303_VOICES) return NUM_303_VOICES - 1;
  return voiceIndex;
}

void MiniAcid::updateSamplesPerStep() {
  samplesPerStep = sampleRateValue * 60.0f / (bpmValue * 4.0f);
}

float MiniAcid::noteToFreq(int note) {
  return 440.0f * powf(2.0f, (note - 69) / 12.0f);
}

void MiniAcid::advanceStep() {
  currentStepIndex = (currentStepIndex + 1) % SEQ_STEPS;

  // 303 voices
  int8_t note = pattern303[currentStepIndex];
  bool accent = pattern303Accent[currentStepIndex];
  bool slide = pattern303Slide[currentStepIndex];

  int8_t note2 = pattern303_2[currentStepIndex];
  bool accent2 = pattern303Accent2[currentStepIndex];
  bool slide2 = pattern303Slide2[currentStepIndex];

  if (!mute303 && note >= 0)
    voice303.startNote(noteToFreq(note), accent, slide);
  else
    voice303.release();

  if (!mute303_2 && note2 >= 0)
    voice3032.startNote(noteToFreq(note2), accent2, slide2);
  else
    voice3032.release();

  // Drums
  if (patternKick[currentStepIndex] && !muteKick)
    drums.triggerKick();
  if (patternSnare[currentStepIndex] && !muteSnare)
    drums.triggerSnare();
  if (patternHat[currentStepIndex] && !muteHat)
    drums.triggerHat();
  if (patternOpenHat[currentStepIndex] && !muteOpenHat)
    drums.triggerOpenHat();
  if (patternMidTom[currentStepIndex] && !muteMidTom)
    drums.triggerMidTom();
  if (patternHighTom[currentStepIndex] && !muteHighTom)
    drums.triggerHighTom();
  if (patternRim[currentStepIndex] && !muteRim)
    drums.triggerRim();
  if (patternClap[currentStepIndex] && !muteClap)
    drums.triggerClap();
}

void MiniAcid::generateAudioBuffer(int16_t *buffer, size_t numSamples) {
  if (!buffer || numSamples == 0) {
    return;
  }

  updateSamplesPerStep();
  delay303.setBpm(bpmValue);
  delay3032.setBpm(bpmValue);

  for (size_t i = 0; i < numSamples; ++i) {
    if (playing) {
      if (samplesIntoStep >= (unsigned long)samplesPerStep) {
        samplesIntoStep = 0;
        advanceStep();
      }
      samplesIntoStep++;
    }

    float sample = 0.0f;
    if (playing) {
      float sample303 = 0.0f;
      if (!mute303) {
        float v = voice303.process() * 0.5f;
        sample303 += delay303.process(v);
      } else {
        // keep delay line ticking even while muted to let tails decay
        delay303.process(0.0f);
      }
      if (!mute303_2) {
        float v = voice3032.process() * 0.5f;
        sample303 += delay3032.process(v);
      } else {
        delay3032.process(0.0f);
      }
      if (!muteKick)
        sample += drums.processKick();
      if (!muteSnare)
        sample += drums.processSnare();
      if (!muteHat)
        sample += drums.processHat();
      if (!muteOpenHat)
        sample += drums.processOpenHat();
      if (!muteMidTom)
        sample += drums.processMidTom();
      if (!muteHighTom)
        sample += drums.processHighTom();
      if (!muteRim)
        sample += drums.processRim();
      if (!muteClap)
        sample += drums.processClap();
      sample += sample303;
    }

    // Soft clipping/limiting
    sample *= 0.65f;
    if (sample > 1.0f)
      sample = 1.0f;
    if (sample < -1.0f)
      sample = -1.0f;

    buffer[i] = static_cast<int16_t>(sample * 32767.0f);
  }

  size_t copyCount = numSamples;
  if (copyCount > AUDIO_BUFFER_SAMPLES) copyCount = AUDIO_BUFFER_SAMPLES;
  for (size_t i = 0; i < copyCount; ++i) lastBuffer[i] = buffer[i];
  lastBufferCount = copyCount;
}

void MiniAcid::randomize303Pattern(int voiceIndex) {
  int idx = clamp303Voice(voiceIndex);
  if (idx == 0)
    PatternGenerator::generateRandom303Pattern(SEQ_STEPS, pattern303, pattern303Accent, pattern303Slide);
  else
    PatternGenerator::generateRandom303Pattern(SEQ_STEPS, pattern303_2, pattern303Accent2, pattern303Slide2);
}

void MiniAcid::randomizeDrumPattern() {
  PatternGenerator::generateRandomDrumPattern(SEQ_STEPS, patternKick, patternSnare, patternHat, patternOpenHat, patternMidTom, patternHighTom, patternRim, patternClap);
}


int dorian_intervals[7] = {0, 2, 3, 5, 7, 9, 10};
int phrygian_intervals[7] = {0, 1, 3, 5, 7, 8, 10};

void PatternGenerator::generateRandom303Pattern(int seq_steps, int8_t *pattern303, bool *pattern303Accent, bool *pattern303Slide) {
  int rootNote = 26;

  for (int i = 0; i < seq_steps; ++i) {
    int r = rand() % 10;
    if (r < 7) {
      pattern303[i] = rootNote + dorian_intervals[rand() % 7] + 12 * (rand() % 3);
    } else {
      pattern303[i] = -1; // 30% chance of rest
    }

    // Random accent (30% chance)
    pattern303Accent[i] = (rand() % 100) < 30;

    // Random slide (20% chance)
    pattern303Slide[i] = (rand() % 100) < 20;
  }
}

void PatternGenerator::generateRandomDrumPattern(int seq_steps, bool *patternKick, bool *patternSnare, bool *patternHat, bool *patternOpenHat, bool *patternMidTom, bool *patternHighTom, bool *patternRim, bool *patternClap) {
  for (int i = 0; i < seq_steps; ++i) {
    // Kick on beats and random additional hits
    if (i % 4 == 0 || (rand() % 100) < 20) {
      patternKick[i] = true;
    } else {
      patternKick[i] = false;
    }

    // Snare on off-beats and random additional hits
    if (i % 4 == 2 || (rand() % 100) < 15) {
      patternSnare[i] = (rand() % 100) < 80; // 80% chance if triggered
    } else {
      patternSnare[i] = false;
    }

    // Hi-hat on every step with occasional openings
    if ((rand() % 100) < 90) {
      patternHat[i] = (rand() % 100) < 80; // 80% chance if triggered
    } else {
      patternHat[i] = false;
    }

    // Open hat occasionally replaces or augments closed hats
    bool wantsOpen = (i % 4 == 3 && (rand() % 100) < 65) || ((rand() % 100) < 20 && patternHat[i]);
    patternOpenHat[i] = wantsOpen;
    if (patternOpenHat[i]) {
      // leave more room when an open hat rings out
      patternHat[i] = false;
    }

    // Mid tom on beat 3-ish with some swing
    patternMidTom[i] = (i % 8 == 4 && (rand() % 100) < 75) || ((rand() % 100) < 8);

    // High tom on beat 4 or random fill
    patternHighTom[i] = (i % 8 == 6 && (rand() % 100) < 70) || ((rand() % 100) < 6);

    // Rim shot sparse
    patternRim[i] = (i % 4 == 1 && (rand() % 100) < 25);

    // Clap on 2 and 4 style
    if (i % 4 == 2) {
      patternClap[i] = (rand() % 100) < 80;
    } else {
      patternClap[i] = (rand() % 100) < 5;
    }
  }
}
