/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.

Voice changer in calls: the effect on the microphone's sound, before WebRTC
gets it, as Novagram Android's org.fenixuz.voice.CallVoiceChanger.
*/
#pragma once

#include "fenix/voice/voice_effect.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

namespace Fenix::Voice {

// The effect chosen for calls, handed over from the main thread to a call's
// audio thread, which looks for a change every buffer.
class CallVoiceSource final {
public:
	// Any thread.
	void set(const Preset &preset);

	// Whether there is a preset newer than [version]; if there is, gives it
	// in [preset] and its version in [version].
	[[nodiscard]] bool takeChange(int &version, Preset &preset) const;

private:
	mutable std::mutex _mutex;
	Preset _preset;
	std::atomic<int> _version = 0;

};

// Changes the microphone's buffers in place. WebRTC needs every buffer back
// at its own size, but an effect gives back what it has rather than what it
// got (the pitch shifters hold some back), so the voice goes through a small
// queue: it starts once it holds a buffer to spare, and from then on a
// buffer goes out for every buffer that comes in. A new choice applies at
// once, with a new effect. One per recording, used from one thread.
class CallVoiceProcessor final {
public:
	explicit CallVoiceProcessor(std::shared_ptr<CallVoiceSource> source);
	~CallVoiceProcessor();

	// [count] samples of 16-bit mono sound at [sampleRate].
	void process(std::int16_t *samples, int count, int sampleRate);

private:
	void enqueue(const std::vector<std::int16_t> &samples);
	void drop(int count);

	const std::shared_ptr<CallVoiceSource> _source;
	int _version = 0;
	Preset _preset;
	int _sampleRate = 0;
	std::unique_ptr<Effect> _effect;
	std::vector<std::int16_t> _output;
	std::vector<std::int16_t> _queue;
	int _queued = 0;
	bool _started = false;

};

} // namespace Fenix::Voice
