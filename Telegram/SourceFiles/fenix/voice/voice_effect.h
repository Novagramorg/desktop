/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.

Voice changer for voice messages: the effects, as on Novagram Android
(org.fenixuz.voice.VoiceEffect), with the same ids, numbers and sound.
*/
#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace Fenix::Voice {

class Sonic;

// Ids are stored in the settings and match Android's, so never renumber.
inline constexpr auto kOff = 0;
inline constexpr auto kEcho = 10;
inline constexpr auto kModulation = 14;
inline constexpr auto kNoise = 15;

// The picker's "Record speed" row, standing for the four speeds (16..19).
inline constexpr auto kSpeedRow = -1;

// What the user sets for the three effects that have settings, in the steps
// Android offers: Modulation's pitch in semitones, Echo's delay in tenths of
// a second and its volume in tenths, Noise's volume in tenths.
struct Params {
	int tone = 5;
	int echoDelay = 5;
	int echoVolume = 7;
	int noiseVolume = 7;

	friend inline bool operator==(const Params &, const Params &) = default;
};

inline constexpr auto kToneMin = -12;
inline constexpr auto kToneMax = 12;
inline constexpr auto kStepMin = 1;
inline constexpr auto kStepMax = 10;

// One effect: a pitch shift or a tape-speed change, plus a few simple
// stages after it. [gain] brings each back to about the voice's loudness.
struct Preset {
	int id = kOff;
	float semitones = 0.f;

	// Where the formants end up, in semitones, apart from the pitch; none
	// leaves them moving with the pitch, the cartoon voice.
	std::optional<float> formant;

	// Tape speed: above 1 faster and thinner, below 1 slower and thicker;
	// the message's length changes.
	float rate = 1.f;

	// Robot: the voice rebuilt as a monotone buzz at this pitch.
	float robotHz = 0.f;

	// Ring modulation: the voice times a sine of [ringHz].
	float ringHz = 0.f;
	float ringMix = 0.f;

	// Feedback echo, each repeat a little duller.
	float echoMs = 0.f;
	float echoFeedback = 0.f;
	float echoMix = 0.f;

	// Freeverb: wet level, room size and damping (0..1), pre-delay.
	float reverbMix = 0.f;
	float reverbRoom = 0.5f;
	float reverbDamp = 0.5f;
	float reverbPreDelayMs = 0.f;

	// Soft clipping, 0 for none.
	float drive = 0.f;

	// Hoarseness: the voice turned to breath, and a rough flutter.
	float whisper = 0.f;
	float roughness = 0.f;

	// Constant soft background noise.
	float noise = 0.f;

	float gain = 1.f;

	[[nodiscard]] bool active() const {
		return (id != kOff);
	}

	friend inline bool operator==(const Preset &, const Preset &) = default;
};

// The picker's rows in Android's order, kSpeedRow standing for the speeds.
[[nodiscard]] const std::vector<int> &RowIds();

// The four tape speeds, thickest first.
[[nodiscard]] const std::vector<int> &SpeedIds();

[[nodiscard]] bool IsSpeed(int id);
[[nodiscard]] bool HasSettings(int id);

// Whether the id is one of the effects (an unknown id reads as off).
[[nodiscard]] bool IsKnown(int id);

// The effect [id] with [params] applied; an unknown id is off.
[[nodiscard]] Preset Resolve(int id, const Params &params);

// The effect chosen for voice messages now. Main thread only.
[[nodiscard]] Preset ChosenForMessages();

// The effect chosen for calls now; a tape speed can't change a live voice,
// so it reads as off. Main thread only.
[[nodiscard]] Preset ChosenForCalls();

// Runs 16-bit mono PCM through a preset, chunk by chunk, as it is recorded.
// The output may be longer or shorter than the input: the pitch shifter
// holds back about two pitch periods, a tape speed changes the length, and
// drain() returns the rest once the recording stops, with an echo or a
// reverb dying away. One instance per recording, used from one thread.
class Effect final {
public:
	Effect(const Preset &preset, int sampleRate);
	~Effect();

	[[nodiscard]] const Preset &preset() const {
		return _preset;
	}

	// Appends the output for [count] samples of [input] to [output].
	void process(
		const std::int16_t *input,
		int count,
		std::vector<std::int16_t> &output);

	// Appends what is still held to [output]; the effect is done after it.
	void drain(std::vector<std::int16_t> &output);

private:
	class FormantWarp;
	class Spectral;
	class Freeverb;

	void warped(
		const std::int16_t *input,
		int count,
		std::vector<std::int16_t> &output);
	void takeSonicOutput(std::vector<std::int16_t> &output);
	void applyStages(std::int16_t *samples, int count);
	[[nodiscard]] float nextNoise();

	const Preset _preset;
	const int _sampleRate = 0;

	std::unique_ptr<FormantWarp> _warp;
	float _inputScale = 1.f;
	int _warpLagLeft = 0;

	std::unique_ptr<Sonic> _sonic;

	double _ringStep = 0.;
	double _ringPhase = 0.;

	std::vector<float> _echo;
	int _echoPosition = 0;
	float _echoDamp = 0.f;

	std::unique_ptr<Freeverb> _reverb;
	std::unique_ptr<Spectral> _spectral;
	int _spectralLag = 0;
	int _fadeLength = 0;

	double _roughPhase = 0.;
	float _noiseLow = 0.f;
	std::uint32_t _noiseSeed = 0x2545F491U;

	std::vector<std::int16_t> _buffer;
	bool _drained = false;

};

} // namespace Fenix::Voice
