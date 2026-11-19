/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.

Sonic audio stream processor for time and pitch stretching, mono only.

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
#pragma once

#include <cstdint>
#include <vector>

namespace Fenix::Voice {

// A C++ port of androidx.media3's Sonic, which Novagram Android's voice
// changer uses, for the same voice on both. Only one channel, and the pitch
// search uses 64-bit sums, as the original C library does: the Java code's
// int products overflow on loud input (undefined behavior in C++). That only
// changes which of equally good pitch periods is picked on loud input; the
// output is otherwise the same as Android's, sample for sample.
class Sonic final {
public:
	Sonic(int inputSampleRate, float speed, float pitch, int outputSampleRate);

	void queueInput(const std::int16_t *samples, int count);

	// Pushes out what is still held, with no delay added.
	void queueEndOfStream();

	[[nodiscard]] int outputCount() const;

	// Moves up to [count] samples of output to [buffer], returns how many.
	int readOutput(std::int16_t *buffer, int count);

private:
	static void EnsureSpace(
		std::vector<std::int16_t> &buffer,
		int frameCount,
		int additionalFrameCount);

	void removeProcessedInputFrames(int position);
	void copyToOutput(
		const std::vector<std::int16_t> &samples,
		int position,
		int frameCount);
	int copyInputToOutput(int position);
	void downSampleInput(
		const std::vector<std::int16_t> &samples,
		int position,
		int skip);
	int findPitchPeriodInRange(
		const std::vector<std::int16_t> &samples,
		int position,
		int minPeriod,
		int maxPeriod);
	[[nodiscard]] bool previousPeriodBetter(int minDiff, int maxDiff) const;
	int findPitchPeriod(const std::vector<std::int16_t> &samples, int position);
	void moveNewSamplesToPitchBuffer(int originalOutputFrameCount);
	void removePitchFrames(int frameCount);
	[[nodiscard]] std::int16_t interpolate(
		const std::vector<std::int16_t> &in,
		int position,
		std::int64_t oldSampleRate,
		std::int64_t newSampleRate) const;
	void adjustRate(float rate, int originalOutputFrameCount);
	int skipPitchPeriod(
		const std::vector<std::int16_t> &samples,
		int position,
		double speed,
		int period);
	int insertPitchPeriod(
		const std::vector<std::int16_t> &samples,
		int position,
		double speed,
		int period);
	void changeSpeed(double speed);
	void processStreamInput();

	const int _inputSampleRate = 0;
	const float _speed = 1.f;
	const float _pitch = 1.f;
	const float _rate = 1.f;
	const int _minPeriod = 0;
	const int _maxPeriod = 0;
	const int _maxRequiredFrameCount = 0;

	std::vector<std::int16_t> _downSampleBuffer;
	std::vector<std::int16_t> _inputBuffer;
	int _inputFrameCount = 0;
	std::vector<std::int16_t> _outputBuffer;
	int _outputFrameCount = 0;
	std::vector<std::int16_t> _pitchBuffer;
	int _pitchFrameCount = 0;
	int _oldRatePosition = 0;
	int _newRatePosition = 0;

	// Frames to copy straight from the input to the output once they come,
	// when time-stretching needs more than the input holds yet.
	int _remainingInputToCopyFrameCount = 0;

	int _prevPeriod = 0;
	int _prevMinDiff = 0;
	int _minDiff = 0;
	int _maxDiff = 0;
	double _accumulatedSpeedAdjustmentError = 0.;

};

} // namespace Fenix::Voice
