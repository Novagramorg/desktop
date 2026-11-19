/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/voice/voice_effect.h"

#include "fenix/fenix_settings.h"
#include "fenix/voice/voice_sonic.h"

#include <algorithm>
#include <cmath>

namespace Fenix::Voice {
namespace {

constexpr auto kPi = 3.14159265358979323846;

// The formant warp can raise peaks, so it runs at half level and the
// stages bring the level back.
constexpr auto kWarpHeadroom = 0.5f;

// Ids of the effects in Android's picker order (kSpeedRow for the speeds).
const auto kRowIds = std::vector<int>{
	kOff,
	kSpeedRow,
	8, // Robot.
	9, // Alien.
	13, // Hoarseness.
	kModulation,
	1, // Child.
	7, // Mouse.
	3, // Man.
	2, // Woman.
	6, // Monster.
	kEcho,
	kNoise,
	4, // Helium.
	5, // Hexafluoride.
	12, // Cave.
};

const auto kSpeedIds = std::vector<int>{ 16, 17, 18, 19 };

// Effects without settings, as Android defines them; the three with
// settings get them in Resolve().
[[nodiscard]] Preset BasePreset(int id) {
	auto result = Preset();
	result.id = id;
	switch (id) {
	case 8: // Robot.
		result.robotHz = 120.f;
		result.gain = 1.15f;
		break;
	case 9: // Alien.
		result.semitones = 4.f;
		result.ringHz = 140.f;
		result.ringMix = 0.6f;
		result.gain = 1.6f;
		break;
	case 13: // Hoarseness.
		result.whisper = 0.5f;
		result.roughness = 0.15f;
		result.drive = 1.3f;
		break;
	case 1: // Child.
		result.semitones = 12.f;
		result.formant = 5.5f;
		result.gain = 1.1f;
		break;
	case 7: // Mouse.
		result.semitones = 14.f;
		break;
	case 3: // Man.
		result.semitones = -6.f;
		result.formant = -3.f;
		break;
	case 2: // Woman.
		result.semitones = 9.f;
		result.formant = 3.5f;
		result.gain = 1.1f;
		break;
	case 6: // Monster.
		result.semitones = -8.f;
		result.formant = -5.f;
		result.drive = 2.5f;
		result.ringHz = 30.f;
		result.ringMix = 0.25f;
		result.gain = 0.54f;
		break;
	case 4: // Helium: mostly the formants go up, the pitch only a little.
		result.semitones = 3.f;
		result.formant = 9.f;
		result.gain = 0.95f;
		break;
	case 5: // Hexafluoride: the other way, a heavy gas.
		result.semitones = -3.f;
		result.formant = -7.f;
		result.gain = 1.08f;
		break;
	case 12: // Cave.
		result.reverbMix = 0.3f;
		result.reverbRoom = 0.7f;
		result.reverbDamp = 0.3f;
		result.reverbPreDelayMs = 35.f;
		break;
	case 16: result.rate = 0.7f; break;
	case 17: result.rate = 0.85f; break;
	case 18: result.rate = 1.25f; break;
	case 19: result.rate = 1.5f; break;
	}
	return result;
}

// Kotlin's Float.roundToInt().
[[nodiscard]] int RoundToInt(float value) {
	return int(std::floor(value + 0.5f));
}

// Feedback loops decay into denormal floats in silence, which are very
// slow on x86; anything this small is far below hearing anyway.
[[nodiscard]] float Flush(float value) {
	return (std::abs(value) < 1e-20f) ? 0.f : value;
}

// Linear up to 0.9, then bends smoothly toward 1, so peaks never clip hard.
[[nodiscard]] float SoftLimit(float x) {
	const auto a = std::abs(x);
	if (a <= 0.9f) {
		return x;
	}
	const auto bent = 0.9f + 0.1f * float(std::tanh((a - 0.9f) / 0.1f));
	return (x < 0.f) ? -bent : bent;
}

// The next xorshift state; as a signed 32-bit value over 2^31 it is white
// noise in -1..1.
[[nodiscard]] std::uint32_t Xorshift(std::uint32_t seed) {
	seed ^= (seed << 13);
	seed ^= (seed >> 17);
	seed ^= (seed << 5);
	return seed;
}

[[nodiscard]] std::vector<float> HannWindow(int size) {
	auto result = std::vector<float>(size);
	for (auto i = 0; i < size; ++i) {
		result[i] = float(0.5 - 0.5 * std::cos(2. * kPi * i / size));
	}
	return result;
}

// In-place radix-2 complex FFT of a fixed power-of-two size; the inverse is
// scaled by 1/size.
class Fft final {
public:
	explicit Fft(int size)
	: _size(size)
	, _cos(size / 2)
	, _sin(size / 2)
	, _reversed(size) {
		for (auto i = 0; i < size / 2; ++i) {
			_cos[i] = float(std::cos(2. * kPi * i / size));
			_sin[i] = float(std::sin(2. * kPi * i / size));
		}
		auto j = 0;
		for (auto i = 1; i < size; ++i) {
			auto bit = size >> 1;
			while (j & bit) {
				j ^= bit;
				bit >>= 1;
			}
			j ^= bit;
			_reversed[i] = j;
		}
	}

	void transform(float *re, float *im, bool inverse) const {
		for (auto i = 0; i < _size; ++i) {
			const auto j = _reversed[i];
			if (i < j) {
				std::swap(re[i], re[j]);
				std::swap(im[i], im[j]);
			}
		}
		for (auto length = 2; length <= _size; length <<= 1) {
			const auto half = length / 2;
			const auto step = _size / length;
			for (auto start = 0; start < _size; start += length) {
				for (auto k = 0; k < half; ++k) {
					const auto wr = _cos[k * step];
					const auto wi = inverse ? _sin[k * step] : -_sin[k * step];
					const auto a = start + k;
					const auto b = a + half;
					const auto tr = re[b] * wr - im[b] * wi;
					const auto ti = re[b] * wi + im[b] * wr;
					re[b] = re[a] - tr;
					im[b] = im[a] - ti;
					re[a] += tr;
					im[a] += ti;
				}
			}
		}
		if (inverse) {
			const auto scale = 1.f / _size;
			for (auto i = 0; i < _size; ++i) {
				re[i] *= scale;
				im[i] *= scale;
			}
		}
	}

private:
	const int _size = 0;
	std::vector<float> _cos;
	std::vector<float> _sin;
	std::vector<int> _reversed;

};

} // namespace

// Moves the formants (the resonances of the throat and mouth) by [factor]
// along the frequency axis and leaves the pitch where it is. In each STFT
// frame the spectral envelope is the log spectrum smoothed through the
// cepstrum: only the quefrencies below 1.5 ms stay, shorter than the pitch
// period of any speaking voice, so no pitch detection is needed. The frame
// is filtered by the warped envelope over its own. 75% overlapping Hann
// windows; the output lags the input by size() samples.
class Effect::FormantWarp final {
public:
	FormantWarp(float factor, int sampleRate)
	: _factor(factor)
	, _size((sampleRate > 32000) ? 2048 : 1024)
	, _hop(_size / 4)
	, _half(_size / 2)
	, _lifter(std::max(8, RoundToInt(sampleRate * kLifterSeconds)))
	, _window(HannWindow(_size))
	, _input(_size)
	, _output(_size)
	, _re(_size)
	, _im(_size)
	, _cepstrumRe(_size)
	, _cepstrumIm(_size)
	, _envelope(_half + 1)
	, _fft(_size)
	// Hann analysis and synthesis at 75% overlap add up to 1.5.
	, _scale(_hop / (0.375f * _size)) {
	}

	[[nodiscard]] int size() const {
		return _size;
	}

	float process(float x) {
		_input[_position] = x;
		const auto y = _output[_position];
		_output[_position] = 0.f;
		_position = (_position + 1) % _size;
		if (++_sinceFrame == _hop) {
			_sinceFrame = 0;
			frame();
		}
		return y;
	}

private:
	// Half the pitch period of a 330 Hz voice, so even a high voice's
	// harmonics stay out of the envelope.
	static constexpr auto kLifterSeconds = 0.0015f;
	static constexpr auto kLifterTaper = 16;

	// ln(8): at most 18 dB up or down, so that a quiet valley or the noise
	// between words isn't blown up.
	static constexpr auto kMaxLogGain = 2.0794415f;

	void frame() {
		for (auto i = 0; i < _size; ++i) {
			_re[i] = _input[(_position + i) % _size] * _window[i];
			_im[i] = 0.f;
		}
		_fft.transform(_re.data(), _im.data(), false);

		// The log magnitude, an even real sequence, through the inverse
		// transform: the cepstrum.
		for (auto k = 0; k <= _half; ++k) {
			const auto level = std::log(std::hypot(_re[k], _im[k]) + 1e-9f);
			_cepstrumRe[k] = level;
			_cepstrumIm[k] = 0.f;
			if (k >= 1 && k < _half) {
				_cepstrumRe[_size - k] = level;
				_cepstrumIm[_size - k] = 0.f;
			}
		}
		_fft.transform(_cepstrumRe.data(), _cepstrumIm.data(), true);
		for (auto n = 0; n < _size; ++n) {
			const auto quefrency = (n <= _half) ? n : (_size - n);
			_cepstrumRe[n] *= (quefrency < _lifter)
				? 1.f
				: (quefrency < _lifter + kLifterTaper)
				? float(0.5 + 0.5 * std::cos(
					kPi * (quefrency - _lifter) / kLifterTaper))
				: 0.f;
			_cepstrumIm[n] = 0.f;
		}
		_fft.transform(_cepstrumRe.data(), _cepstrumIm.data(), false);
		for (auto k = 0; k <= _half; ++k) {
			_envelope[k] = _cepstrumRe[k];
		}

		for (auto k = 0; k <= _half; ++k) {
			// The envelope [factor] times lower lands here: above 1 the
			// formants go up.
			const auto from = k / _factor;
			const auto below = int(from);
			const auto target = (below >= _half)
				? _envelope[_half]
				: (_envelope[below]
					+ (_envelope[below + 1] - _envelope[below])
						* (from - below));
			const auto gain = std::exp(std::clamp(
				target - _envelope[k],
				-kMaxLogGain,
				kMaxLogGain));
			_re[k] *= gain;
			_im[k] *= gain;
			if (k >= 1 && k < _half) {
				_re[_size - k] = _re[k];
				_im[_size - k] = -_im[k];
			}
		}
		_fft.transform(_re.data(), _im.data(), true);
		for (auto i = 0; i < _size; ++i) {
			_output[(_position + i) % _size] += _re[i] * _window[i] * _scale;
		}
	}

	const float _factor = 1.f;
	const int _size = 0;
	const int _hop = 0;
	const int _half = 0;
	const int _lifter = 0;
	const std::vector<float> _window;
	std::vector<float> _input;
	std::vector<float> _output;
	std::vector<float> _re;
	std::vector<float> _im;
	std::vector<float> _cepstrumRe;
	std::vector<float> _cepstrumIm;
	std::vector<float> _envelope;
	const Fft _fft;
	const float _scale = 1.f;
	int _position = 0;
	int _sinceFrame = 0;

};

// Short-time Fourier processing for the effects that rebuild the voice from
// its spectrum (DAFX, Zolzer, ch. 7.4): a robot keeps each frame's spectrum
// but drops its phases, so every frame becomes one pulse and the voice takes
// the monotone pitch of the frame rate; a whisper gives the phases random
// values, which turns the voice into breath that keeps its vowels. Hann
// windows on both sides; the output lags the input by size() samples, and
// dryDelayed() is the input with the same lag, for mixing.
class Effect::Spectral final {
public:
	Spectral(bool robot, int size, int hop)
	: _robot(robot)
	, _size(size)
	, _hop(std::max(hop, 1))
	, _window(HannWindow(size))
	, _input(size)
	, _output(size)
	, _re(size)
	, _im(size)
	, _fft(size)
	// Keeps the level: overlapping frames add up in phase for the robot's
	// pulses, at random for the whisper.
	, _scale(robot
		? (_hop / (0.375f * size))
		: std::sqrt(_hop / (0.140625f * size))) {
	}

	[[nodiscard]] int size() const {
		return _size;
	}
	[[nodiscard]] float dryDelayed() const {
		return _dryDelayed;
	}

	float process(float x) {
		_dryDelayed = _input[_position];
		_input[_position] = x;
		const auto y = _output[_position];
		_output[_position] = 0.f;
		_position = (_position + 1) % _size;
		if (++_sinceFrame == _hop) {
			_sinceFrame = 0;
			frame();
		}
		return y;
	}

private:
	// Analyses the last size() samples and adds the rebuilt frame to the
	// output, starting with the next sample.
	void frame() {
		for (auto i = 0; i < _size; ++i) {
			_re[i] = _input[(_position + i) % _size] * _window[i];
			_im[i] = 0.f;
		}
		_fft.transform(_re.data(), _im.data(), false);
		const auto half = _size / 2;
		_re[0] = 0.f;
		_im[0] = 0.f;
		for (auto k = 1; k <= half; ++k) {
			const auto magnitude = std::hypot(_re[k], _im[k]);
			if (_robot) {
				// Zero phase, moved to the middle of the frame (the sign
				// flip of odd bins) where the window is open.
				_re[k] = (k % 2 == 0) ? magnitude : -magnitude;
				_im[k] = 0.f;
			} else {
				_seed = Xorshift(_seed);
				const auto angle = std::int32_t(_seed) / 2147483648. * kPi;
				_re[k] = magnitude * float(std::cos(angle));
				_im[k] = (k == half)
					? 0.f
					: magnitude * float(std::sin(angle));
			}
			if (k < half) {
				_re[_size - k] = _re[k];
				_im[_size - k] = -_im[k];
			}
		}
		_fft.transform(_re.data(), _im.data(), true);
		for (auto i = 0; i < _size; ++i) {
			_output[(_position + i) % _size] += _re[i] * _window[i] * _scale;
		}
	}

	const bool _robot = false;
	const int _size = 0;
	const int _hop = 0;
	const std::vector<float> _window;
	std::vector<float> _input;
	std::vector<float> _output;
	std::vector<float> _re;
	std::vector<float> _im;
	const Fft _fft;
	const float _scale = 1.f;
	int _position = 0;
	int _sinceFrame = 0;
	std::uint32_t _seed = 0x6C8E9CF5U;
	float _dryDelayed = 0.f;

};

// Freeverb (Jezar at Dreampoint, public domain): eight damped feedback
// combs in parallel, then four allpasses in series. The usual tunings, given
// for 44.1 kHz, are scaled to the sample rate. Mono, wet signal only.
class Effect::Freeverb final {
public:
	Freeverb(int sampleRate, const Preset &preset)
	: _feedback(preset.reverbRoom * 0.28f + 0.7f)
	, _damp(preset.reverbDamp * 0.4f)
	// A one-pole high-pass at 200 Hz on what goes into the reverb, so low
	// notes don't pile up into a boom.
	, _highPassCoef(float(std::exp(-2. * kPi * 200. / sampleRate))) {
		const auto scale = sampleRate / 44100.f;
		for (const auto length : { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 }) {
			_combs.push_back({ std::vector<float>(
				std::max(1, int(length * scale))) });
		}
		for (const auto length : { 556, 441, 341, 225 }) {
			_allpasses.push_back({ std::vector<float>(
				std::max(1, int(length * scale))) });
		}
		if (preset.reverbPreDelayMs > 0.f) {
			_preDelay.resize(std::max(
				1,
				int(sampleRate * preset.reverbPreDelayMs / 1000.f)));
		}
	}

	float process(float dry) {
		_highPassOut = Flush(
			_highPassCoef * (_highPassOut + dry - _highPassIn));
		_highPassIn = dry;
		auto input = _highPassOut;
		if (!_preDelay.empty()) {
			const auto delayed = _preDelay[_preDelayPosition];
			_preDelay[_preDelayPosition] = input;
			_preDelayPosition = (_preDelayPosition + 1)
				% int(_preDelay.size());
			input = delayed;
		}
		input *= 0.015f * 2.f;
		auto out = 0.f;
		for (auto &comb : _combs) {
			const auto y = comb.line[comb.position];
			comb.store = Flush(y * (1.f - _damp) + comb.store * _damp);
			comb.line[comb.position] = Flush(input + comb.store * _feedback);
			comb.position = (comb.position + 1) % int(comb.line.size());
			out += y;
		}
		for (auto &allpass : _allpasses) {
			const auto buffered = allpass.line[allpass.position];
			allpass.line[allpass.position] = Flush(out + buffered * 0.5f);
			allpass.position = (allpass.position + 1)
				% int(allpass.line.size());
			out = buffered - out;
		}
		return out * 3.f;
	}

private:
	struct Comb {
		std::vector<float> line;
		int position = 0;
		float store = 0.f;
	};
	struct Allpass {
		std::vector<float> line;
		int position = 0;
	};

	const float _feedback = 0.f;
	const float _damp = 0.f;
	const float _highPassCoef = 0.f;
	std::vector<Comb> _combs;
	std::vector<Allpass> _allpasses;
	std::vector<float> _preDelay;
	int _preDelayPosition = 0;
	float _highPassOut = 0.f;
	float _highPassIn = 0.f;

};

const std::vector<int> &RowIds() {
	return kRowIds;
}

const std::vector<int> &SpeedIds() {
	return kSpeedIds;
}

bool IsSpeed(int id) {
	return ranges::contains(kSpeedIds, id);
}

bool HasSettings(int id) {
	return (id == kModulation) || (id == kEcho) || (id == kNoise);
}

bool IsKnown(int id) {
	return (id != kSpeedRow)
		&& (ranges::contains(kRowIds, id) || IsSpeed(id));
}

Preset Resolve(int id, const Params &params) {
	auto result = BasePreset(IsKnown(id) ? id : kOff);
	switch (result.id) {
	case kModulation: {
		// A pitch the user picks. The formants go half as far, between a
		// cartoon voice (all the way) and another person's (not at all);
		// at 0 the voice passes as it is.
		const auto tone = std::clamp(params.tone, kToneMin, kToneMax);
		if (tone) {
			result.semitones = float(tone);
			result.formant = tone / 2.f;
		}
	} break;
	case kEcho: {
		const auto delay = std::clamp(params.echoDelay, kStepMin, kStepMax);
		const auto volume = std::clamp(params.echoVolume, kStepMin, kStepMax)
			/ 10.f;
		const auto mix = volume * 0.8f;
		result.echoMs = delay * 100.f;
		result.echoMix = mix;
		result.echoFeedback = 0.25f + 0.3f * volume;
		result.gain = 1.f / (1.f + 0.35f * mix);
	} break;
	case kNoise: {
		const auto volume = std::clamp(params.noiseVolume, kStepMin, kStepMax);
		result.noise = 0.03f * volume / 10.f;
	} break;
	}
	return result;
}

Preset ChosenForMessages() {
	const auto choice = Fenix::Get().voiceChanger();
	return Resolve(choice.preset, Params{
		.tone = choice.tone,
		.echoDelay = choice.echoDelay,
		.echoVolume = choice.echoVolume,
		.noiseVolume = choice.noiseVolume,
	});
}

Effect::Effect(const Preset &preset, int sampleRate)
: _preset(preset)
, _sampleRate(sampleRate) {
	if (_preset.formant) {
		// Sonic moves the formants along with the pitch, so the warp moves
		// them only by what is left over.
		_warp = std::make_unique<FormantWarp>(
			std::pow(2.f, (*_preset.formant - _preset.semitones) / 12.f),
			sampleRate);
		_inputScale = 1.f / kWarpHeadroom;
		_warpLagLeft = _warp->size();
	}
	if (_preset.semitones != 0.f || _preset.rate != 1.f) {
		// Pitch keeps the length; tape speed resamples to rate / speed, so
		// the same samples play faster or slower.
		_sonic = std::make_unique<Sonic>(
			sampleRate,
			1.f,
			std::pow(2.f, _preset.semitones / 12.f),
			RoundToInt(sampleRate / _preset.rate));
	}
	_ringStep = 2. * kPi * _preset.ringHz / sampleRate;
	if (_preset.echoMs > 0.f) {
		_echo.resize(std::max(1, int(sampleRate * _preset.echoMs / 1000.f)));
	}
	if (_preset.reverbMix > 0.f) {
		_reverb = std::make_unique<Freeverb>(sampleRate, _preset);
	}
	if (_preset.robotHz > 0.f) {
		_spectral = std::make_unique<Spectral>(
			true,
			1024,
			RoundToInt(sampleRate / _preset.robotHz));
	} else if (_preset.whisper > 0.f) {
		_spectral = std::make_unique<Spectral>(false, 512, 128);
	}
	// What drain() adds after the voice: the spectral stage's lag, then a
	// second for an echo or a reverb to fade.
	_spectralLag = _spectral ? _spectral->size() : 0;
	_fadeLength = (!_echo.empty() || _reverb) ? sampleRate : 0;
}

Effect::~Effect() = default;

void Effect::process(
		const std::int16_t *input,
		int count,
		std::vector<std::int16_t> &output) {
	if (count <= 0 || _drained) {
		return;
	}
	const auto start = output.size();
	if (!_warp && !_sonic) {
		output.insert(output.end(), input, input + count);
	} else {
		auto source = input;
		auto sourceCount = count;
		if (_warp) {
			_buffer.clear();
			warped(input, count, _buffer);
			source = _buffer.data();
			sourceCount = int(_buffer.size());
		}
		if (_sonic) {
			_sonic->queueInput(source, sourceCount);
			takeSonicOutput(output);
		} else {
			output.insert(output.end(), source, source + sourceCount);
		}
	}
	applyStages(output.data() + start, int(output.size() - start));
}

void Effect::drain(std::vector<std::int16_t> &output) {
	if (_drained) {
		return;
	}
	_drained = true;

	const auto start = output.size();

	// Silence through the warp first: it still holds the last of the voice.
	_buffer.clear();
	if (_warp) {
		const auto silence = std::vector<std::int16_t>(_warp->size());
		warped(silence.data(), int(silence.size()), _buffer);
	}
	if (_sonic) {
		_sonic->queueInput(_buffer.data(), int(_buffer.size()));
		_sonic->queueEndOfStream();
		takeSonicOutput(output);
	} else {
		output.insert(output.end(), _buffer.begin(), _buffer.end());
	}
	const auto rest = output.size() - start;

	// Silence through the stages brings out what they still hold; the echo
	// or reverb part fades to nothing.
	output.resize(output.size() + _spectralLag + _fadeLength, 0);
	applyStages(output.data() + start, int(output.size() - start));
	const auto fadeStart = start + rest + _spectralLag;
	for (auto i = 0; i < _fadeLength; ++i) {
		auto &sample = output[fadeStart + i];
		sample = std::int16_t(int(
			sample * (1.f - float(i) / _fadeLength)));
	}
}

void Effect::warped(
		const std::int16_t *input,
		int count,
		std::vector<std::int16_t> &output) {
	// What the warp gives for its first size() samples is the silence
	// before the voice reached it.
	const auto skip = std::min(_warpLagLeft, count);
	for (auto i = 0; i < count; ++i) {
		const auto y = _warp->process(input[i] / 32768.f) * kWarpHeadroom;
		if (i >= skip) {
			output.push_back(std::int16_t(int(
				std::clamp(y * 32767.f, -32768.f, 32767.f))));
		}
	}
	_warpLagLeft -= skip;
}

void Effect::takeSonicOutput(std::vector<std::int16_t> &output) {
	const auto count = _sonic->outputCount();
	if (count <= 0) {
		return;
	}
	const auto start = output.size();
	output.resize(start + count);
	const auto read = _sonic->readOutput(output.data() + start, count);
	output.resize(start + read);
}

void Effect::applyStages(std::int16_t *samples, int count) {
	const auto &p = _preset;
	for (auto i = 0; i < count; ++i) {
		auto x = samples[i] / 32768.f * _inputScale;

		if (_spectral) {
			const auto wet = _spectral->process(x);
			x = (p.robotHz > 0.f)
				? wet
				: (_spectral->dryDelayed() * (1.f - p.whisper)
					+ wet * p.whisper);
		}
		if (p.ringMix > 0.f) {
			const auto ring = x * float(std::sin(_ringPhase));
			x = x * (1.f - p.ringMix) + ring * p.ringMix;
			_ringPhase += _ringStep;
			if (_ringPhase > 2. * kPi) {
				_ringPhase -= 2. * kPi;
			}
		}
		if (p.roughness > 0.f) {
			// An irregular 30..50 Hz flutter of the loudness, like a
			// strained throat.
			_roughPhase += 2. * kPi * (40. + 10. * nextNoise()) / _sampleRate;
			if (_roughPhase > 2. * kPi) {
				_roughPhase -= 2. * kPi;
			}
			x *= 1.f - p.roughness
				* (0.5f + 0.5f * float(std::sin(_roughPhase)));
		}
		if (p.drive > 0.f) {
			x = float(std::tanh(double(x * p.drive))
				/ std::tanh(double(p.drive)));
		}
		if (!_echo.empty()) {
			// A one-pole low-pass in the loop: each repeat loses some
			// treble, like a wall far away.
			const auto past = _echo[_echoPosition];
			_echoDamp = Flush(_echoDamp + (past - _echoDamp) * 0.6f);
			_echo[_echoPosition] = Flush(x + _echoDamp * p.echoFeedback);
			_echoPosition = (_echoPosition + 1) % int(_echo.size());
			x += _echoDamp * p.echoMix;
		}
		if (_reverb) {
			x = x * (1.f - p.reverbMix * 0.5f)
				+ _reverb->process(x) * p.reverbMix;
		}
		if (p.noise > 0.f) {
			// Softened (low-passed) noise, a hiss rather than a hard white
			// noise.
			_noiseLow += (nextNoise() - _noiseLow) * 0.35f;
			x += _noiseLow * p.noise * 2.f;
		}

		x *= p.gain;
		samples[i] = std::int16_t(int(SoftLimit(x) * 32767.f));
	}
}

float Effect::nextNoise() {
	_noiseSeed = Xorshift(_noiseSeed);
	return std::int32_t(_noiseSeed) / 2147483648.f;
}

} // namespace Fenix::Voice
