/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/voice/voice_call.h"

#include <algorithm>
#include <cstring>

namespace Fenix::Voice {

void CallVoiceSource::set(const Preset &preset) {
	const auto lock = std::lock_guard(_mutex);
	_preset = preset;
	_version.fetch_add(1, std::memory_order_release);
}

bool CallVoiceSource::takeChange(int &version, Preset &preset) const {
	if (_version.load(std::memory_order_acquire) == version) {
		return false;
	}
	const auto lock = std::lock_guard(_mutex);
	preset = _preset;
	version = _version.load(std::memory_order_relaxed);
	return true;
}

CallVoiceProcessor::CallVoiceProcessor(
	std::shared_ptr<CallVoiceSource> source)
: _source(std::move(source)) {
}

CallVoiceProcessor::~CallVoiceProcessor() = default;

void CallVoiceProcessor::process(
		std::int16_t *samples,
		int count,
		int sampleRate) {
	if (!samples || count <= 0 || sampleRate <= 0) {
		return;
	}
	auto rebuild = _source && _source->takeChange(_version, _preset);
	if (_sampleRate != sampleRate) {
		_sampleRate = sampleRate;
		rebuild = true;
	}
	if (rebuild) {
		_effect = _preset.active()
			? std::make_unique<Effect>(_preset, _sampleRate)
			: nullptr;
		_queued = 0;
		_started = false;
	}
	if (!_effect) {
		return;
	}
	_output.clear();
	_effect->process(samples, count, _output);
	enqueue(_output);

	// Silence until the queue holds a buffer to spare; then a buffer out
	// for every buffer in. Should the effect ever fall behind, the gap is
	// silent, and should it run ahead, the oldest samples go rather than
	// the delay growing. A buffer counts as at least 10 ms here (WebRTC's
	// own size), as the pitch shifters give their output in pieces that
	// big: smaller buffers would make all of it look like running ahead.
	const auto unit = std::max(count, sampleRate / 100);
	if (!_started && _queued >= unit * 2) {
		_started = true;
	}
	if (_queued > unit * 6) {
		drop(_queued - unit * 2);
	}
	if (!_started) {
		std::fill(samples, samples + count, std::int16_t(0));
		return;
	}
	const auto take = std::min(count, _queued);
	std::copy(_queue.data(), _queue.data() + take, samples);
	std::fill(samples + take, samples + count, std::int16_t(0));
	drop(take);
}

void CallVoiceProcessor::enqueue(const std::vector<std::int16_t> &samples) {
	const auto count = int(samples.size());
	if (!count) {
		return;
	}
	if (int(_queue.size()) < _queued + count) {
		_queue.resize(std::max(_queue.size() * 2, size_t(_queued + count)));
	}
	std::memcpy(
		_queue.data() + _queued,
		samples.data(),
		count * sizeof(std::int16_t));
	_queued += count;
}

void CallVoiceProcessor::drop(int count) {
	count = std::min(count, _queued);
	if (count <= 0) {
		return;
	}
	std::memmove(
		_queue.data(),
		_queue.data() + count,
		(_queued - count) * sizeof(std::int16_t));
	_queued -= count;
}

} // namespace Fenix::Voice
