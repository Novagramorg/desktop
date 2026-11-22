/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.

Voice changer in calls: a call's audio device module, with the microphone's
sound going through the voice changer before WebRTC gets it. Built in td_ui,
which sees WebRTC's headers; this header needs none of them.
*/
#pragma once

#include <functional>
#include <memory>

namespace webrtc {
class AudioDeviceModule;
class TaskQueueFactory;
template <class T>
class scoped_refptr;
} // namespace webrtc

namespace Fenix::Voice {

class CallVoiceSource;

using AudioDeviceModuleCreator = std::function<
	webrtc::scoped_refptr<webrtc::AudioDeviceModule>(
		webrtc::TaskQueueFactory*)>;

// [creator]'s module, the microphone going through the effect [source]
// holds. Only a mono microphone is changed: a stereo capture (the system's
// sound, when shared) passes as it is.
[[nodiscard]] AudioDeviceModuleCreator WrapCallAudioDeviceModule(
	AudioDeviceModuleCreator creator,
	std::shared_ptr<CallVoiceSource> source);

} // namespace Fenix::Voice
