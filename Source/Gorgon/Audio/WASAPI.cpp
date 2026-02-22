#undef UNICODE

#include "../Audio.h"
#include "../OS/Win32Unicode.h"
#include <initguid.h>
#include <Audioclient.h>
#include <Mmdeviceapi.h>
#include <comdef.h>
#include <functiondiscoverykeys_devpkey.h>

namespace Gorgon {

namespace Audio {

	IMMDeviceEnumerator *Enumerator = nullptr;
	IMMDevice *CurrentDevice = nullptr;
	IAudioClient *AudioClient = nullptr;
	IAudioRenderClient *RenderClient = nullptr;
	bool available = false;

	void AudioLoop();

	constexpr int REFTIMES_PER_SECONDS = 10000000;

	namespace internal {
		extern std::thread audiothread;

		extern int   BufferSize;
		extern float BufferDuration;
	}

	std::vector<Channel> ToChannels(unsigned f) {
		std::vector<Channel> channels;
		if(f & SPEAKER_FRONT_LEFT)
			channels.push_back(Channel::FrontLeft);

		if(f & SPEAKER_FRONT_RIGHT)
			channels.push_back(Channel::FrontRight);

		if(f & SPEAKER_FRONT_CENTER)
			channels.push_back(Channel::Center);

		if(f & SPEAKER_BACK_LEFT)
			channels.push_back(Channel::BackLeft);

		if(f & SPEAKER_BACK_RIGHT)
			channels.push_back(Channel::BackRight);

		if(f & SPEAKER_LOW_FREQUENCY)
			channels.push_back(Channel::LowFreq);

		if(channels.size() == 1 && channels[0] == Channel::Center) {
			channels[0] = Channel::Mono;
		}

		return channels;
	}

	Device GetDevice(IMMDevice *dev) {
		std::string devid, devname;
		std::vector<Channel> channels;
		bool headphones = false;

		wchar_t *id = nullptr;
		dev->GetId(&id);
		if(id!=nullptr) {
			devid = UnicodeToMByte(id);
			CoTaskMemFree(id);
		}

		IPropertyStore *props = nullptr;

		dev->OpenPropertyStore(STGM_READ, &props);

		if(props == nullptr) {
			return {};
		}

		PROPVARIANT pv;

		props->GetValue(PKEY_Device_FriendlyName, &pv);

		devname = UnicodeToMByte(pv.pwszVal);

		props->GetValue(PKEY_AudioEndpoint_FormFactor, &pv);

		if(pv.uintVal == 3) {
			headphones=true;
			channels ={Channel::FrontLeft, Channel::FrontRight};
		}
		else {
			props->GetValue(PKEY_AudioEndpoint_PhysicalSpeakers, &pv);

			channels = ToChannels(pv.uintVal);
		}

		props->GetValue(PKEY_AudioEngine_DeviceFormat, &pv);

		auto fmt = *(WAVEFORMATEX*)pv.blob.pBlobData;

		if(channels.size() == 0) {
			if(fmt.wFormatTag==WAVE_FORMAT_EXTENSIBLE) {
				auto efmt = *(WAVEFORMATEXTENSIBLE*)pv.blob.pBlobData;
				channels = ToChannels(efmt.dwChannelMask);
			}
			else {
				channels = StandardChannels(fmt.nChannels);
			}
		}

		props->Release();

		return Device(
			devid, devname, fmt.nSamplesPerSec,
			Format::Float, headphones, channels
		);
	}

	void Initialize() {
		HRESULT hr;

		hr = CoInitialize(NULL);

		if(FAILED(hr)) {
			Log.Log("Cannot initialize COM subsystem.");
			return;
		}

		hr = CoCreateInstance(
			__uuidof(MMDeviceEnumerator), NULL,
			CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
			(void**)&Enumerator);

		if(FAILED(hr)) {
			Log.Log("Cannot create enumerator.");
			return;
		}

		hr = Enumerator->GetDefaultAudioEndpoint(
			eRender, eConsole, &CurrentDevice);

		if(FAILED(hr)) {
			Log.Log("Cannot get default device.");
			return;
		}

		hr = CurrentDevice->Activate(
			__uuidof(IAudioClient), CLSCTX_ALL,
			NULL, (void**)&AudioClient);


		if(FAILED(hr)) {
			Log.Log("Cannot activate default device.");
			return;
		}

		WAVEFORMATEX *wavefmt;

		hr = AudioClient->GetMixFormat(&wavefmt);

		if(FAILED(hr)) {
			Log.Log("Cannot get playback format.");
			return;
		}

		if(wavefmt->wFormatTag != WAVE_FORMAT_EXTENSIBLE) {
			Log.Log("Unexpected device format.");
			return;
		}

		WAVEFORMATEXTENSIBLE *fmt = (WAVEFORMATEXTENSIBLE *)wavefmt;

		fmt->SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;

		hr = AudioClient->Initialize(
			AUDCLNT_SHAREMODE_SHARED,
			0,
			long(internal::BufferDuration * REFTIMES_PER_SECONDS),
			0,
			wavefmt,
			NULL
		);

		if(FAILED(hr)) {
			Log.Log("Cannot initialize audio client.");
			return;
		}

		hr = AudioClient->GetService(
			__uuidof(IAudioRenderClient),
			(void**)&RenderClient);

		if(FAILED(hr)) {
			Log.Log("Cannot get audio stream.");
			return;
		}

		// Get the actual size of the allocated buffer.
		UINT32 bs = 0;
		hr = AudioClient->GetBufferSize(&bs);

		internal::BufferSize = bs;
		
		if(FAILED(hr) || bs == 0) {
			Log.Log("Cannot obtain audio buffer size.");
			return;
		}

		Device::Refresh();

		auto temp = GetDevice(CurrentDevice);
		
		Current = Device(
			temp.GetID(), temp.GetName(),
			fmt->Format.nSamplesPerSec,
			Format::Float, temp.IsHeadphones(), 
			ToChannels(fmt->dwChannelMask)
		);

		internal::volume.resize(Current.GetChannelCount());
		for(auto &v : internal::volume) v = 1;

		internal::audiothread = std::thread(&AudioLoop);
		SetThreadPriority(internal::audiothread.native_handle(), THREAD_PRIORITY_HIGHEST);

		hr = AudioClient->Start();

		if(FAILED(hr)) {
			Log.Log("Cannot start audio stream.");
			return;
		}

		Log.Log("WASAPI Audio subsystem is ready.", Utils::Logger::Success);

		available = true;
	}

	void Device::Refresh() {
		HRESULT hr;

		IMMDeviceCollection *devs = nullptr;

		IMMDevice *defaultdev = nullptr;
		hr = Enumerator->GetDefaultAudioEndpoint(
			eRender, eConsole, &defaultdev);

		std::string defaultdevid;

		wchar_t *id = nullptr;
		defaultdev->GetId(&id);
		if(id!=nullptr) {
			defaultdevid = UnicodeToMByte(id);
			CoTaskMemFree(id);
		}

		devices.clear();

		hr = Enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &devs);

		if(FAILED(hr)) {
			Log << "Device enumeration is failed.";
			return;
		}

		unsigned count = 0;
		devs->GetCount(&count);

		for(unsigned i=0; i<count; i++) {
			IMMDevice *dev = nullptr;
			devs->Item(i, &dev);

			auto device = GetDevice(dev);

			dev->Release();

			if(device.id == "")
				continue;

			devices.push_back(device);

			if(devices.back().id == defaultdevid)
				def = devices.back();
		}

		devs->Release();
	}


	bool IsAvailable() {
		return available;
	}

}
}