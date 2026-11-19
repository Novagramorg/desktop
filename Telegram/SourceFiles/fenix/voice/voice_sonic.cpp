/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.

Copyright (C) 2017 The Android Open Source Project
Copyright (C) 2010 Bill Cox, Sonic Library (https://github.com/waywardgeek/sonic)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#include "fenix/voice/voice_sonic.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace Fenix::Voice {
namespace {

constexpr auto kMinimumPitch = 65;
constexpr auto kMaximumPitch = 400;
constexpr auto kAmdfFrequency = 4000;

constexpr auto kMinimumSpeedupRate = 1.00001f;
constexpr auto kMinimumSlowdownRate = 0.99999f;

// Java's Math.round(double).
[[nodiscard]] int JavaRound(double value) {
	return int(std::floor(value + 0.5));
}

void OverlapAdd(
		int frameCount,
		std::int16_t *out,
		const std::int16_t *rampDown,
		const std::int16_t *rampUp) {
	for (auto t = 0; t < frameCount; ++t) {
		out[t] = std::int16_t((int(rampDown[t]) * (frameCount - t)
			+ int(rampUp[t]) * t) / frameCount);
	}
}

} // namespace

Sonic::Sonic(
	int inputSampleRate,
	float speed,
	float pitch,
	int outputSampleRate)
: _inputSampleRate(inputSampleRate)
, _speed(speed)
, _pitch(pitch)
, _rate(float(inputSampleRate) / outputSampleRate)
, _minPeriod(inputSampleRate / kMaximumPitch)
, _maxPeriod(inputSampleRate / kMinimumPitch)
, _maxRequiredFrameCount(2 * _maxPeriod)
, _downSampleBuffer(_maxRequiredFrameCount)
, _inputBuffer(_maxRequiredFrameCount)
, _outputBuffer(_maxRequiredFrameCount)
, _pitchBuffer(_maxRequiredFrameCount) {
}

void Sonic::queueInput(const std::int16_t *samples, int count) {
	if (count <= 0) {
		return;
	}
	EnsureSpace(_inputBuffer, _inputFrameCount, count);
	std::copy(samples, samples + count, _inputBuffer.begin() + _inputFrameCount);
	_inputFrameCount += count;
	processStreamInput();
}

void Sonic::queueEndOfStream() {
	const auto remainingFrameCount = _inputFrameCount;
	const auto s = double(_speed / _pitch);
	const auto r = double(_rate * _pitch);

	// Frames to be copied straight to the output aren't processed, so they
	// don't count as input here.
	const auto adjustedRemainingFrames = remainingFrameCount
		- _remainingInputToCopyFrameCount;
	const auto expectedOutputFrames = _outputFrameCount
		+ int((adjustedRemainingFrames / s
			+ _remainingInputToCopyFrameCount
			+ _accumulatedSpeedAdjustmentError
			+ _pitchFrameCount) / r + 0.5);
	_accumulatedSpeedAdjustmentError = 0.;

	// Enough silence to flush both the input and the pitch buffers.
	const auto silence = 2 * _maxRequiredFrameCount;
	EnsureSpace(_inputBuffer, _inputFrameCount, remainingFrameCount + silence);
	std::fill(
		_inputBuffer.begin() + remainingFrameCount,
		_inputBuffer.begin() + remainingFrameCount + silence,
		std::int16_t(0));
	_inputFrameCount += silence;
	processStreamInput();

	// Throw away what the added silence made.
	if (_outputFrameCount > expectedOutputFrames) {
		_outputFrameCount = std::max(expectedOutputFrames, 0);
	}
	_inputFrameCount = 0;
	_remainingInputToCopyFrameCount = 0;
	_pitchFrameCount = 0;
}

int Sonic::outputCount() const {
	return _outputFrameCount;
}

int Sonic::readOutput(std::int16_t *buffer, int count) {
	const auto frames = std::clamp(count, 0, _outputFrameCount);
	std::copy(_outputBuffer.begin(), _outputBuffer.begin() + frames, buffer);
	_outputFrameCount -= frames;
	std::copy(
		_outputBuffer.begin() + frames,
		_outputBuffer.begin() + frames + _outputFrameCount,
		_outputBuffer.begin());
	return frames;
}

void Sonic::EnsureSpace(
		std::vector<std::int16_t> &buffer,
		int frameCount,
		int additionalFrameCount) {
	const auto capacity = int(buffer.size());
	if (frameCount + additionalFrameCount > capacity) {
		buffer.resize(3 * capacity / 2 + additionalFrameCount);
	}
}

void Sonic::removeProcessedInputFrames(int position) {
	const auto remaining = _inputFrameCount - position;
	std::copy(
		_inputBuffer.begin() + position,
		_inputBuffer.begin() + position + remaining,
		_inputBuffer.begin());
	_inputFrameCount = remaining;
}

void Sonic::copyToOutput(
		const std::vector<std::int16_t> &samples,
		int position,
		int frameCount) {
	EnsureSpace(_outputBuffer, _outputFrameCount, frameCount);
	std::copy(
		samples.begin() + position,
		samples.begin() + position + frameCount,
		_outputBuffer.begin() + _outputFrameCount);
	_outputFrameCount += frameCount;
}

int Sonic::copyInputToOutput(int position) {
	const auto frameCount = std::min(
		_maxRequiredFrameCount,
		_remainingInputToCopyFrameCount);
	copyToOutput(_inputBuffer, position, frameCount);
	_remainingInputToCopyFrameCount -= frameCount;
	return frameCount;
}

void Sonic::downSampleInput(
		const std::vector<std::int16_t> &samples,
		int position,
		int skip) {
	// Averages [skip] samples together into the down-sample buffer.
	const auto frameCount = _maxRequiredFrameCount / skip;
	for (auto i = 0; i < frameCount; ++i) {
		auto value = 0;
		for (auto j = 0; j < skip; ++j) {
			value += samples[position + i * skip + j];
		}
		value /= skip;
		_downSampleBuffer[i] = std::int16_t(value);
	}
}

int Sonic::findPitchPeriodInRange(
		const std::vector<std::int16_t> &samples,
		int position,
		int minPeriod,
		int maxPeriod) {
	// The best frequency match in the range (AMDF).
	auto bestPeriod = 0;
	auto worstPeriod = 255;
	auto minDiff = std::int64_t(1);
	auto maxDiff = std::int64_t(0);
	for (auto period = minPeriod; period <= maxPeriod; ++period) {
		auto diff = std::int64_t(0);
		for (auto i = 0; i < period; ++i) {
			const auto sVal = int(samples[position + i]);
			const auto pVal = int(samples[position + period + i]);
			diff += std::abs(sVal - pVal);
		}
		if (diff * bestPeriod < minDiff * period) {
			minDiff = diff;
			bestPeriod = period;
		}
		if (diff * worstPeriod > maxDiff * period) {
			maxDiff = diff;
			worstPeriod = period;
		}
	}
	if (!bestPeriod) {
		// Only for an empty range, which the callers never pass.
		_minDiff = 0;
		_maxDiff = 0;
		return minPeriod;
	}
	_minDiff = int(minDiff / bestPeriod);
	_maxDiff = int(maxDiff / worstPeriod);
	return bestPeriod;
}

bool Sonic::previousPeriodBetter(int minDiff, int maxDiff) const {
	// The previous estimate may be better at the abrupt end of a word.
	if (minDiff == 0 || _prevPeriod == 0) {
		return false;
	} else if (maxDiff > minDiff * 3) {
		// A reasonable match this period.
		return false;
	} else if (minDiff * 2 <= _prevMinDiff * 3) {
		// Not that much worse a mismatch this period.
		return false;
	}
	return true;
}

int Sonic::findPitchPeriod(
		const std::vector<std::int16_t> &samples,
		int position) {
	// Down sampled to about 4 kHz first for speed, then searched again
	// around that without down sampling.
	auto period = 0;
	const auto skip = (_inputSampleRate > kAmdfFrequency)
		? (_inputSampleRate / kAmdfFrequency)
		: 1;
	if (skip == 1) {
		period = findPitchPeriodInRange(
			samples,
			position,
			_minPeriod,
			_maxPeriod);
	} else {
		downSampleInput(samples, position, skip);
		period = findPitchPeriodInRange(
			_downSampleBuffer,
			0,
			_minPeriod / skip,
			_maxPeriod / skip);
		period *= skip;
		const auto minP = std::max(period - (skip * 4), _minPeriod);
		const auto maxP = std::min(period + (skip * 4), _maxPeriod);
		period = findPitchPeriodInRange(samples, position, minP, maxP);
	}
	const auto result = previousPeriodBetter(_minDiff, _maxDiff)
		? _prevPeriod
		: period;
	_prevMinDiff = _minDiff;
	_prevPeriod = period;
	return result;
}

void Sonic::moveNewSamplesToPitchBuffer(int originalOutputFrameCount) {
	const auto frameCount = _outputFrameCount - originalOutputFrameCount;
	EnsureSpace(_pitchBuffer, _pitchFrameCount, frameCount);
	std::copy(
		_outputBuffer.begin() + originalOutputFrameCount,
		_outputBuffer.begin() + originalOutputFrameCount + frameCount,
		_pitchBuffer.begin() + _pitchFrameCount);
	_outputFrameCount = originalOutputFrameCount;
	_pitchFrameCount += frameCount;
}

void Sonic::removePitchFrames(int frameCount) {
	if (frameCount <= 0) {
		return;
	}
	std::copy(
		_pitchBuffer.begin() + frameCount,
		_pitchBuffer.begin() + _pitchFrameCount,
		_pitchBuffer.begin());
	_pitchFrameCount -= frameCount;
}

std::int16_t Sonic::interpolate(
		const std::vector<std::int16_t> &in,
		int position,
		std::int64_t oldSampleRate,
		std::int64_t newSampleRate) const {
	const auto left = std::int64_t(in[position]);
	const auto right = std::int64_t(in[position + 1]);
	const auto now = std::int64_t(_newRatePosition) * oldSampleRate;
	const auto leftPosition = std::int64_t(_oldRatePosition) * newSampleRate;
	const auto rightPosition = (std::int64_t(_oldRatePosition) + 1)
		* newSampleRate;
	const auto ratio = rightPosition - now;
	const auto width = rightPosition - leftPosition;
	return std::int16_t((ratio * left + (width - ratio) * right) / width);
}

void Sonic::adjustRate(float rate, int originalOutputFrameCount) {
	if (_outputFrameCount == originalOutputFrameCount) {
		return;
	}
	auto newSampleRate = std::int64_t(_inputSampleRate / rate);
	auto oldSampleRate = std::int64_t(_inputSampleRate);
	// Smaller numbers for the integer math.
	while (newSampleRate != 0
		&& oldSampleRate != 0
		&& newSampleRate % 2 == 0
		&& oldSampleRate % 2 == 0) {
		newSampleRate /= 2;
		oldSampleRate /= 2;
	}
	moveNewSamplesToPitchBuffer(originalOutputFrameCount);
	// Leave at least one pitch sample in the buffer.
	for (auto position = 0; position < _pitchFrameCount - 1; ++position) {
		while ((std::int64_t(_oldRatePosition) + 1) * newSampleRate
			> std::int64_t(_newRatePosition) * oldSampleRate) {
			EnsureSpace(_outputBuffer, _outputFrameCount, 1);
			_outputBuffer[_outputFrameCount] = interpolate(
				_pitchBuffer,
				position,
				oldSampleRate,
				newSampleRate);
			++_newRatePosition;
			++_outputFrameCount;
		}
		++_oldRatePosition;
		if (_oldRatePosition == oldSampleRate) {
			_oldRatePosition = 0;
			_newRatePosition = 0;
		}
	}
	removePitchFrames(_pitchFrameCount - 1);
}

int Sonic::skipPitchPeriod(
		const std::vector<std::int16_t> &samples,
		int position,
		double speed,
		int period) {
	// Skips a pitch period and copies period/speed samples to the output.
	auto newFrameCount = 0;
	if (speed >= 2.) {
		const auto expected = period / (speed - 1.)
			+ _accumulatedSpeedAdjustmentError;
		newFrameCount = JavaRound(expected);
		_accumulatedSpeedAdjustmentError = expected - newFrameCount;
	} else {
		newFrameCount = period;
		const auto expected = period * (2. - speed) / (speed - 1.)
			+ _accumulatedSpeedAdjustmentError;
		_remainingInputToCopyFrameCount = JavaRound(expected);
		_accumulatedSpeedAdjustmentError = expected
			- _remainingInputToCopyFrameCount;
	}
	EnsureSpace(_outputBuffer, _outputFrameCount, newFrameCount);
	OverlapAdd(
		newFrameCount,
		_outputBuffer.data() + _outputFrameCount,
		samples.data() + position,
		samples.data() + position + period);
	_outputFrameCount += newFrameCount;
	return newFrameCount;
}

int Sonic::insertPitchPeriod(
		const std::vector<std::int16_t> &samples,
		int position,
		double speed,
		int period) {
	// Inserts a pitch period and decides how much input to copy directly.
	auto newFrameCount = 0;
	if (speed < 0.5) {
		const auto expected = period * speed / (1. - speed)
			+ _accumulatedSpeedAdjustmentError;
		newFrameCount = JavaRound(expected);
		_accumulatedSpeedAdjustmentError = expected - newFrameCount;
	} else {
		newFrameCount = period;
		const auto expected = period * (2. * speed - 1.) / (1. - speed)
			+ _accumulatedSpeedAdjustmentError;
		_remainingInputToCopyFrameCount = JavaRound(expected);
		_accumulatedSpeedAdjustmentError = expected
			- _remainingInputToCopyFrameCount;
	}
	EnsureSpace(_outputBuffer, _outputFrameCount, period + newFrameCount);
	std::copy(
		samples.begin() + position,
		samples.begin() + position + period,
		_outputBuffer.begin() + _outputFrameCount);
	OverlapAdd(
		newFrameCount,
		_outputBuffer.data() + _outputFrameCount + period,
		samples.data() + position + period,
		samples.data() + position);
	_outputFrameCount += period + newFrameCount;
	return newFrameCount;
}

void Sonic::changeSpeed(double speed) {
	if (_inputFrameCount < _maxRequiredFrameCount) {
		return;
	}
	const auto frameCount = _inputFrameCount;
	auto position = 0;
	do {
		if (_remainingInputToCopyFrameCount > 0) {
			position += copyInputToOutput(position);
		} else {
			const auto period = findPitchPeriod(_inputBuffer, position);
			if (speed > 1.) {
				position += period
					+ skipPitchPeriod(_inputBuffer, position, speed, period);
			} else {
				position += insertPitchPeriod(
					_inputBuffer,
					position,
					speed,
					period);
			}
		}
	} while (position + _maxRequiredFrameCount <= frameCount);
	removeProcessedInputFrames(position);
}

void Sonic::processStreamInput() {
	// Resamples as many pitch periods as the input holds.
	const auto originalOutputFrameCount = _outputFrameCount;
	const auto s = double(_speed / _pitch);
	const auto r = _rate * _pitch;
	if (s > kMinimumSpeedupRate || s < kMinimumSlowdownRate) {
		changeSpeed(s);
	} else {
		copyToOutput(_inputBuffer, 0, _inputFrameCount);
		_inputFrameCount = 0;
	}
	if (r != 1.f) {
		adjustRate(r, originalOutputFrameCount);
	}
}

} // namespace Fenix::Voice
