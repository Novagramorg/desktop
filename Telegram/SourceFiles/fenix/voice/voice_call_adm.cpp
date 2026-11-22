/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/voice/voice_call_adm.h"

#include "fenix/voice/voice_call.h"

#include <api/make_ref_counted.h>
#include <modules/audio_device/include/audio_device.h>
#include <modules/audio_device/include/audio_device_defines.h>

#include <atomic>
#include <cstring>
#include <vector>

namespace Fenix::Voice {
namespace {

// Hands everything to the module it wraps, which keeps doing the work; only
// the microphone's sound is changed on its way to WebRTC (the audio
// transport), the same way webrtc's own ADMWrapper watches it. Not final:
// rtc::make_ref_counted derives from it.
class VoiceChangerModule
	: public webrtc::AudioDeviceModule
	, public webrtc::AudioTransport {
public:
	VoiceChangerModule(
		webrtc::scoped_refptr<webrtc::AudioDeviceModule> inner,
		std::shared_ptr<CallVoiceSource> source);
	~VoiceChangerModule() override;

	// webrtc::AudioTransport.
	int32_t RecordedDataIsAvailable(
		const void *audioSamples,
		size_t nSamples,
		size_t nBytesPerSample,
		size_t nChannels,
		uint32_t samplesPerSec,
		uint32_t totalDelayMS,
		int32_t clockDrift,
		uint32_t currentMicLevel,
		bool keyPressed,
		uint32_t &newMicLevel) override;
	int32_t RecordedDataIsAvailable(
		const void *audioSamples,
		size_t nSamples,
		size_t nBytesPerSample,
		size_t nChannels,
		uint32_t samplesPerSec,
		uint32_t totalDelayMS,
		int32_t clockDrift,
		uint32_t currentMicLevel,
		bool keyPressed,
		uint32_t &newMicLevel,
		absl::optional<int64_t> estimatedCaptureTimeNS) override;
	int32_t NeedMorePlayData(
		size_t nSamples,
		size_t nBytesPerSample,
		size_t nChannels,
		uint32_t samplesPerSec,
		void *audioSamples,
		size_t &nSamplesOut,
		int64_t *elapsed_time_ms,
		int64_t *ntp_time_ms) override;
	void PullRenderData(
		int bits_per_sample,
		int sample_rate,
		size_t number_of_channels,
		size_t number_of_frames,
		void *audio_data,
		int64_t *elapsed_time_ms,
		int64_t *ntp_time_ms) override;

	// webrtc::AudioDeviceModule.
	int32_t RegisterAudioCallback(
		webrtc::AudioTransport *audioCallback) override;

	int32_t ActiveAudioLayer(AudioLayer *audioLayer) const override {
		return _inner->ActiveAudioLayer(audioLayer);
	}
	int32_t Init() override {
		return _inner->Init();
	}
	int32_t Terminate() override {
		return _inner->Terminate();
	}
	bool Initialized() const override {
		return _inner->Initialized();
	}
	int16_t PlayoutDevices() override {
		return _inner->PlayoutDevices();
	}
	int16_t RecordingDevices() override {
		return _inner->RecordingDevices();
	}
	int32_t PlayoutDeviceName(
			uint16_t index,
			char name[webrtc::kAdmMaxDeviceNameSize],
			char guid[webrtc::kAdmMaxGuidSize]) override {
		return _inner->PlayoutDeviceName(index, name, guid);
	}
	int32_t RecordingDeviceName(
			uint16_t index,
			char name[webrtc::kAdmMaxDeviceNameSize],
			char guid[webrtc::kAdmMaxGuidSize]) override {
		return _inner->RecordingDeviceName(index, name, guid);
	}
	int32_t SetPlayoutDevice(uint16_t index) override {
		return _inner->SetPlayoutDevice(index);
	}
	int32_t SetPlayoutDevice(WindowsDeviceType device) override {
		return _inner->SetPlayoutDevice(device);
	}
	int32_t SetRecordingDevice(uint16_t index) override {
		return _inner->SetRecordingDevice(index);
	}
	int32_t SetRecordingDevice(WindowsDeviceType device) override {
		return _inner->SetRecordingDevice(device);
	}
	int32_t PlayoutIsAvailable(bool *available) override {
		return _inner->PlayoutIsAvailable(available);
	}
	int32_t InitPlayout() override {
		return _inner->InitPlayout();
	}
	bool PlayoutIsInitialized() const override {
		return _inner->PlayoutIsInitialized();
	}
	int32_t RecordingIsAvailable(bool *available) override {
		return _inner->RecordingIsAvailable(available);
	}
	int32_t InitRecording() override {
		return _inner->InitRecording();
	}
	bool RecordingIsInitialized() const override {
		return _inner->RecordingIsInitialized();
	}
	int32_t StartPlayout() override {
		return _inner->StartPlayout();
	}
	int32_t StopPlayout() override {
		return _inner->StopPlayout();
	}
	bool Playing() const override {
		return _inner->Playing();
	}
	int32_t StartRecording() override {
		return _inner->StartRecording();
	}
	int32_t StopRecording() override {
		return _inner->StopRecording();
	}
	bool Recording() const override {
		return _inner->Recording();
	}
	int32_t InitSpeaker() override {
		return _inner->InitSpeaker();
	}
	bool SpeakerIsInitialized() const override {
		return _inner->SpeakerIsInitialized();
	}
	int32_t InitMicrophone() override {
		return _inner->InitMicrophone();
	}
	bool MicrophoneIsInitialized() const override {
		return _inner->MicrophoneIsInitialized();
	}
	int32_t SpeakerVolumeIsAvailable(bool *available) override {
		return _inner->SpeakerVolumeIsAvailable(available);
	}
	int32_t SetSpeakerVolume(uint32_t volume) override {
		return _inner->SetSpeakerVolume(volume);
	}
	int32_t SpeakerVolume(uint32_t *volume) const override {
		return _inner->SpeakerVolume(volume);
	}
	int32_t MaxSpeakerVolume(uint32_t *maxVolume) const override {
		return _inner->MaxSpeakerVolume(maxVolume);
	}
	int32_t MinSpeakerVolume(uint32_t *minVolume) const override {
		return _inner->MinSpeakerVolume(minVolume);
	}
	int32_t MicrophoneVolumeIsAvailable(bool *available) override {
		return _inner->MicrophoneVolumeIsAvailable(available);
	}
	int32_t SetMicrophoneVolume(uint32_t volume) override {
		return _inner->SetMicrophoneVolume(volume);
	}
	int32_t MicrophoneVolume(uint32_t *volume) const override {
		return _inner->MicrophoneVolume(volume);
	}
	int32_t MaxMicrophoneVolume(uint32_t *maxVolume) const override {
		return _inner->MaxMicrophoneVolume(maxVolume);
	}
	int32_t MinMicrophoneVolume(uint32_t *minVolume) const override {
		return _inner->MinMicrophoneVolume(minVolume);
	}
	int32_t SpeakerMuteIsAvailable(bool *available) override {
		return _inner->SpeakerMuteIsAvailable(available);
	}
	int32_t SetSpeakerMute(bool enable) override {
		return _inner->SetSpeakerMute(enable);
	}
	int32_t SpeakerMute(bool *enabled) const override {
		return _inner->SpeakerMute(enabled);
	}
	int32_t MicrophoneMuteIsAvailable(bool *available) override {
		return _inner->MicrophoneMuteIsAvailable(available);
	}
	int32_t SetMicrophoneMute(bool enable) override {
		return _inner->SetMicrophoneMute(enable);
	}
	int32_t MicrophoneMute(bool *enabled) const override {
		return _inner->MicrophoneMute(enabled);
	}
	int32_t StereoPlayoutIsAvailable(bool *available) const override {
		return _inner->StereoPlayoutIsAvailable(available);
	}
	int32_t SetStereoPlayout(bool enable) override {
		return _inner->SetStereoPlayout(enable);
	}
	int32_t StereoPlayout(bool *enabled) const override {
		return _inner->StereoPlayout(enabled);
	}
	int32_t StereoRecordingIsAvailable(bool *available) const override {
		return _inner->StereoRecordingIsAvailable(available);
	}
	int32_t SetStereoRecording(bool enable) override {
		return _inner->SetStereoRecording(enable);
	}
	int32_t StereoRecording(bool *enabled) const override {
		return _inner->StereoRecording(enabled);
	}
	int32_t PlayoutDelay(uint16_t *delayMS) const override {
		return _inner->PlayoutDelay(delayMS);
	}
	bool BuiltInAECIsAvailable() const override {
		return _inner->BuiltInAECIsAvailable();
	}
	bool BuiltInAGCIsAvailable() const override {
		return _inner->BuiltInAGCIsAvailable();
	}
	bool BuiltInNSIsAvailable() const override {
		return _inner->BuiltInNSIsAvailable();
	}
	int32_t EnableBuiltInAEC(bool enable) override {
		return _inner->EnableBuiltInAEC(enable);
	}
	int32_t EnableBuiltInAGC(bool enable) override {
		return _inner->EnableBuiltInAGC(enable);
	}
	int32_t EnableBuiltInNS(bool enable) override {
		return _inner->EnableBuiltInNS(enable);
	}
	int32_t GetPlayoutUnderrunCount() const override {
		return _inner->GetPlayoutUnderrunCount();
	}
	absl::optional<Stats> GetStats() const override {
		return _inner->GetStats();
	}
#if defined(WEBRTC_IOS)
	int GetPlayoutAudioParameters(
			webrtc::AudioParameters *params) const override {
		return _inner->GetPlayoutAudioParameters(params);
	}
	int GetRecordAudioParameters(
			webrtc::AudioParameters *params) const override {
		return _inner->GetRecordAudioParameters(params);
	}
#endif // WEBRTC_IOS

private:
	[[nodiscard]] const void *changed(
		const void *audioSamples,
		size_t nSamples,
		size_t nBytesPerSample,
		size_t nChannels,
		uint32_t samplesPerSec);

	webrtc::scoped_refptr<webrtc::AudioDeviceModule> _inner;
	std::atomic<webrtc::AudioTransport*> _transport = nullptr;

	// The recording thread's own.
	CallVoiceProcessor _processor;
	std::vector<std::int16_t> _samples;

};

VoiceChangerModule::VoiceChangerModule(
	webrtc::scoped_refptr<webrtc::AudioDeviceModule> inner,
	std::shared_ptr<CallVoiceSource> source)
: _inner(std::move(inner))
, _processor(std::move(source)) {
}

VoiceChangerModule::~VoiceChangerModule() {
	// The module we wrap stops its threads as it goes, and may still hand
	// over a last buffer meanwhile: let it go while all of this is alive.
	_inner = nullptr;
}

int32_t VoiceChangerModule::RegisterAudioCallback(
		webrtc::AudioTransport *audioCallback) {
	// The wrapped module refuses while it records or plays, keeping the old
	// one; then this keeps it too.
	const auto result = _inner->RegisterAudioCallback(
		audioCallback ? this : nullptr);
	if (result == 0) {
		_transport.store(audioCallback, std::memory_order_release);
	}
	return result;
}

const void *VoiceChangerModule::changed(
		const void *audioSamples,
		size_t nSamples,
		size_t nBytesPerSample,
		size_t nChannels,
		uint32_t samplesPerSec) {
	// The microphone records mono; nBytesPerSample is per frame.
	if (!audioSamples
		|| !nSamples
		|| nChannels != 1
		|| nBytesPerSample != sizeof(std::int16_t)
		|| !samplesPerSec) {
		return audioSamples;
	}
	_samples.resize(nSamples);
	std::memcpy(
		_samples.data(),
		audioSamples,
		nSamples * sizeof(std::int16_t));
	_processor.process(
		_samples.data(),
		int(nSamples),
		int(samplesPerSec));
	return _samples.data();
}

int32_t VoiceChangerModule::RecordedDataIsAvailable(
		const void *audioSamples,
		size_t nSamples,
		size_t nBytesPerSample,
		size_t nChannels,
		uint32_t samplesPerSec,
		uint32_t totalDelayMS,
		int32_t clockDrift,
		uint32_t currentMicLevel,
		bool keyPressed,
		uint32_t &newMicLevel) {
	return RecordedDataIsAvailable(
		audioSamples,
		nSamples,
		nBytesPerSample,
		nChannels,
		samplesPerSec,
		totalDelayMS,
		clockDrift,
		currentMicLevel,
		keyPressed,
		newMicLevel,
		absl::nullopt);
}

int32_t VoiceChangerModule::RecordedDataIsAvailable(
		const void *audioSamples,
		size_t nSamples,
		size_t nBytesPerSample,
		size_t nChannels,
		uint32_t samplesPerSec,
		uint32_t totalDelayMS,
		int32_t clockDrift,
		uint32_t currentMicLevel,
		bool keyPressed,
		uint32_t &newMicLevel,
		absl::optional<int64_t> estimatedCaptureTimeNS) {
	const auto transport = _transport.load(std::memory_order_acquire);
	if (!transport) {
		return 0;
	}
	return transport->RecordedDataIsAvailable(
		changed(
			audioSamples,
			nSamples,
			nBytesPerSample,
			nChannels,
			samplesPerSec),
		nSamples,
		nBytesPerSample,
		nChannels,
		samplesPerSec,
		totalDelayMS,
		clockDrift,
		currentMicLevel,
		keyPressed,
		newMicLevel,
		estimatedCaptureTimeNS);
}

int32_t VoiceChangerModule::NeedMorePlayData(
		size_t nSamples,
		size_t nBytesPerSample,
		size_t nChannels,
		uint32_t samplesPerSec,
		void *audioSamples,
		size_t &nSamplesOut,
		int64_t *elapsed_time_ms,
		int64_t *ntp_time_ms) {
	const auto transport = _transport.load(std::memory_order_acquire);
	if (!transport) {
		// Safe values for every out parameter, as the interface asks.
		nSamplesOut = 0;
		*elapsed_time_ms = -1;
		*ntp_time_ms = -1;
		return 0;
	}
	return transport->NeedMorePlayData(
		nSamples,
		nBytesPerSample,
		nChannels,
		samplesPerSec,
		audioSamples,
		nSamplesOut,
		elapsed_time_ms,
		ntp_time_ms);
}

void VoiceChangerModule::PullRenderData(
		int bits_per_sample,
		int sample_rate,
		size_t number_of_channels,
		size_t number_of_frames,
		void *audio_data,
		int64_t *elapsed_time_ms,
		int64_t *ntp_time_ms) {
	if (const auto transport = _transport.load(std::memory_order_acquire)) {
		transport->PullRenderData(
			bits_per_sample,
			sample_rate,
			number_of_channels,
			number_of_frames,
			audio_data,
			elapsed_time_ms,
			ntp_time_ms);
	}
}

} // namespace

AudioDeviceModuleCreator WrapCallAudioDeviceModule(
		AudioDeviceModuleCreator creator,
		std::shared_ptr<CallVoiceSource> source) {
	if (!creator || !source) {
		return creator;
	}
	return [=](webrtc::TaskQueueFactory *factory)
	-> webrtc::scoped_refptr<webrtc::AudioDeviceModule> {
		auto inner = creator(factory);
		if (!inner) {
			return inner;
		}
		return rtc::make_ref_counted<VoiceChangerModule>(
			std::move(inner),
			source);
	};
}

} // namespace Fenix::Voice
