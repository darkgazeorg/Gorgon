#include "../Audio.h"

#include <pulse/pulseaudio.h>
#include <thread>
#include <string.h>

#include <unistd.h>

#include "Controllers.h"
#include "../Main.h"

namespace Gorgon :: Audio {
	///@cond Internal
	
	pa_mainloop *pa_main = nullptr;
	pa_context  *pa_ctx  = nullptr;
	pa_stream   *pa_strm = nullptr;
	
	
	namespace internal {
		extern float BufferDuration; //in seconds
		extern int   BufferSize;
	}
	
	void AudioLoop();

	enum { pa_waiting, pa_connected, pa_failed, pa_timeout, pa_done, pa_ready } pa_state=pa_waiting;
	
	bool pa_wait_connection(pa_mainloop *pam, unsigned long timeout = 1000) {
		auto start = Time::GetTime();
		while(pa_state == pa_waiting) {
			pa_mainloop_iterate(pam, 0, NULL);
			
			if(Time::GetTime()-start > timeout) {
				pa_state = pa_timeout;
                Log<<"A wait operation is timed out";
				return false;
			}
		}
		
		return true;
	}
	
	bool pa_wait_stream_connection(pa_mainloop *pam, pa_stream *pa_stream, unsigned long timeout = 1000) {
		auto start = Time::GetTime();
		while(pa_stream_get_state(pa_stream) != PA_STREAM_READY) {
			pa_mainloop_iterate(pam, 0, NULL);
			
			if(Time::GetTime()-start > timeout) {
				pa_state = pa_timeout;
                Log<<"Stream connection is timed out";
				return false;
			}
		}
		
		return true;
	}
	
    bool wait_pa_op(pa_operation* pa_op, unsigned long timeout = 1000) {
        if(pa_op == nullptr) {
            //pa_error
            Log.Log("A wait operation is failed: ")<<pa_strerror(pa_context_errno(pa_ctx));
            return false;
        }
        
		auto start = Time::GetTime();
		
		while(pa_operation_get_state(pa_op) == PA_OPERATION_RUNNING) {
			pa_mainloop_iterate(pa_main, 0, NULL);
			
			if(Time::GetTime()-start > timeout) {
				pa_state = pa_timeout;
                Log<<"A wait operation is timed out";
				return false;
			}
		}
		
		return true;
	}


	void pa_state_cb(pa_context *ctx, void *userdata) {
		pa_context_state_t state;
		
		state = pa_context_get_state(ctx);
		switch  (state) {
			// There are just here for reference
			case PA_CONTEXT_UNCONNECTED:
			case PA_CONTEXT_CONNECTING:
			case PA_CONTEXT_AUTHORIZING:
			case PA_CONTEXT_SETTING_NAME:
			default:
				break;
			case PA_CONTEXT_FAILED:
			case PA_CONTEXT_TERMINATED:
				pa_state = pa_failed;
				break;
			case PA_CONTEXT_READY:
				pa_state=pa_connected;
				break;
		}
	}

	pa_channel_position convertopa(Channel ch) {
        switch(ch) {
        case Channel::FrontLeft:
            return PA_CHANNEL_POSITION_FRONT_LEFT;
        case Channel::FrontRight:
            return PA_CHANNEL_POSITION_FRONT_RIGHT;
        case Channel::BackLeft:
            return PA_CHANNEL_POSITION_REAR_LEFT;
        case Channel::BackRight:
            return PA_CHANNEL_POSITION_REAR_RIGHT;
        case Channel::Mono:
            return PA_CHANNEL_POSITION_MONO;
        case Channel::LowFreq:
            return PA_CHANNEL_POSITION_LFE;
        case Channel::Center:
            return PA_CHANNEL_POSITION_FRONT_CENTER;
        default:
            return PA_CHANNEL_POSITION_INVALID;
        }
    }
	
	Channel convertpachannel(pa_channel_position channel) {
		switch(channel) {
		case PA_CHANNEL_POSITION_FRONT_LEFT:
		//case PA_CHANNEL_POSITION_LEFT:
			return Channel::FrontLeft;
		case PA_CHANNEL_POSITION_FRONT_RIGHT:
		//case PA_CHANNEL_POSITION_RIGHT:
			return Channel::FrontRight;
		case PA_CHANNEL_POSITION_REAR_LEFT:
			return Channel::BackLeft;
		case PA_CHANNEL_POSITION_REAR_RIGHT:
			return Channel::BackRight;
		case PA_CHANNEL_POSITION_FRONT_CENTER:
		//case PA_CHANNEL_POSITION_CENTER:
			return Channel::Center;
		case PA_CHANNEL_POSITION_MONO:
			return Channel::Mono;
		case PA_CHANNEL_POSITION_LFE:
			return Channel::LowFreq;
		default:
			return Channel::Unknown;
		}
	}
	
	void Initialize() {
		Log << "Starting pulse audio initialization...";
		
		//ready structs
		pa_main = pa_mainloop_new();
		
		if(!pa_main) {
			Log.Log("Pulse audio main loop creation failed.");
			pa_state = pa_failed;
			return;
		}
		
		pa_ctx  = pa_context_new(pa_mainloop_get_api(pa_main), GetSystemName().c_str());
		
		if(!pa_ctx) {
			Log.Log("Pulse audio context creation failed.");
			pa_state = pa_failed;
			return;
		}
		
		//set call back
		pa_context_set_state_callback(pa_ctx, pa_state_cb, nullptr);
		
		//try connect
		pa_context_connect(pa_ctx, nullptr, PA_CONTEXT_NOFLAGS, nullptr);
		
		//wait for connection
		pa_wait_connection(pa_main
#ifdef PA_TIMEOUT
			, PA_TIMEOUT
#endif
		);
		
		if(pa_state == pa_failed) {
			Log.Log("Pulse audio connection failed: ") << pa_strerror(pa_context_errno(pa_ctx));
			return;
		}
		
		if(pa_state == pa_timeout) {
			Log.Log("Pulse audio connection timed out");
			return;
		}
		
		//list devices
		Device::Refresh();
		
		if(Device::Devices().size() == 0 || !Device::Default().IsValid()) {
			Log.Log("No audio device found.");
			pa_state = pa_failed;
			return;
		}
		
		//create stream
		pa_sample_spec ss;
		ss.channels = Device::Default().GetChannelCount();
		ss.format   = PA_SAMPLE_FLOAT32LE; //only this mode is supported
		ss.rate     = Device::Default().GetSampleRate();
        
        pa_channel_map chmap = {};
        
        chmap.channels = ss.channels;
        
        for(int i=0; i<chmap.channels; i++) {
            chmap.map[i] = convertopa(Device::Default().GetChannel(i));
        }
		
		pa_strm     = pa_stream_new(pa_ctx, GetSystemName().c_str(), &ss, &chmap);
		
		if(!pa_strm) {
			Log.Log("Cannot create Pulse audio stream") << pa_strerror(pa_context_errno(pa_ctx));
			pa_state = pa_failed;
			return;
		}
		
		
		// obtain default device settings
		auto spec   = pa_stream_get_sample_spec(pa_strm);
        
        pa_buffer_attr attr = {(uint32_t) -1, (uint32_t) -1, (uint32_t) -1, (uint32_t) -1, (uint32_t) -1};
        attr.tlength   = unsigned(spec->rate * internal::BufferDuration) * spec->channels * sizeof(float);
        attr.maxlength = attr.tlength * 1.5;
		
		int result = pa_stream_connect_playback(pa_strm, Device::Default().GetID().c_str(), &attr, PA_STREAM_ADJUST_LATENCY, NULL, NULL);
        
        if(result != 0) {
            
            result = pa_stream_connect_playback(pa_strm, NULL, &attr, PA_STREAM_NOFLAGS, NULL, NULL);
            
            if(result) {
                Log.Log("Cannot connect to stream");
                return;
            }
            else
                Log << "Cannot set playback latency.";
        }
        
        if(!pa_wait_stream_connection(pa_main, pa_strm
#ifdef PA_TIMEOUT
			, PA_TIMEOUT
#endif
        )) return;
        
        auto attrp = pa_stream_get_buffer_attr(pa_strm);
        if(attrp) attr = *attrp;
        
        internal::BufferSize = attr.tlength / sizeof(float) / spec->channels;
		
		auto devname = pa_stream_get_device_name(pa_strm);
		std::string name;
		
		if(devname==nullptr) {
			name = "[Default]";
		}
		else {
			name=devname;
		}
		
		auto newchmap  = pa_stream_get_channel_map(pa_strm);
		
		std::vector<Channel> channels;
		for(int i=0; i<newchmap->channels; i++) {
			channels.push_back(convertpachannel(newchmap->map[i]));
		}		
		
		Current = Device(
			name, 
			Device::Default().GetName(),
			spec->rate,
			Format::Float,
            Device::Default().IsHeadphones(),
			channels
		);
		
		internal::volume.resize(channels.size());
		for(auto &v : internal::volume) v = 1;
		
		Log << "Starting audio loop";
		
		internal::audiothread = std::thread(&AudioLoop);
		
		struct sched_param sp = { 0 };
		sp.sched_priority = 1;
		
		result=pthread_setschedparam(internal::audiothread.native_handle(), SCHED_FIFO, &sp);
		if(result!=0) {
			if(errno == EPERM) {
				Log << "Access denied while setting thread priority, there might be glitches in the audio." << std::endl
				    << "You might want to elevate current user to allow changing process scheduling.";
			}
			else {
				Log << "Error occurred while setting thread priority, there might be glitches in the audio:" << std::endl << strerror(errno);
			}
			
		}
		
		//done
		Log.Log("Pulse audio is ready over ", Utils::Logger::Success)<<name<<" device and available.";
        pa_state = pa_ready;
	}
	
	static std::vector<Device> tempdevices;
	
	void pa_sinklist_cb(pa_context *c, const pa_sink_info *l, int eol, void *userdata) {
		// If eol is set to a positive number, you're at the end of the list
		if (eol > 0) {
			return;
		}
		
		Format format;
		//currently only floating point is supported
		switch(l->sample_spec.format) {
		case PA_SAMPLE_FLOAT32LE:
			format=Format::Float;
			break;
		default:
			format=Format::Float;
			break;
		}
		
		std::vector<Channel> channels;
		for(int i = 0; i<l->channel_map.channels; i++) {
			channels.push_back(convertpachannel(l->channel_map.map[i]));
		}
		
		bool headphones = false;
        if(l->active_port) {
            std::string pname=l->active_port->name;
            headphones = String::ToLower(pname).find("headphone") != pname.npos;
        }
		
		tempdevices.push_back(Device(l->name, l->description, l->sample_spec.rate, format, headphones, channels));
		
		Log << "Found device: "<<l->name;
	}

	void server_info_cb(pa_context* context, const pa_server_info* info, void *nm) {
		std::string &def_sinkname = *(std::string*)nm;
		def_sinkname=info->default_sink_name;
	}
	
	void Device::Refresh() {
		Log << "Probing devices...";
		tempdevices.clear();
		
		auto pa_op = pa_context_get_sink_info_list(pa_ctx, pa_sinklist_cb, nullptr);
        
        if(!pa_op) {
            Log << "Probe failed: " << pa_strerror(pa_context_errno(pa_ctx));
            return;
        }

		bool finished = wait_pa_op(pa_op
#ifdef PA_TIMEOUT
			, PA_TIMEOUT
#endif
		);
		pa_operation_unref(pa_op);
		
		using std::swap;
		swap(tempdevices, devices);
		
		std::string def_sinkname = "";
		pa_op = pa_context_get_server_info(pa_ctx, &server_info_cb, &def_sinkname);
		finished = wait_pa_op(pa_op
#ifdef PA_TIMEOUT
			, PA_TIMEOUT
#endif
		);
		//pa_operation_unref(pa_op);
		
		if(def_sinkname!="") {
			try {
				def = Find(def_sinkname);
			}
			catch(const std::runtime_error &err) {
				Log << err.what();
				
				if(devices.size()) {
					def=devices[0];
				}
			}
		}
		else if(devices.size()) {
			def=devices[0];
		}
		
		if(!finished) {
			Log << "Device probe is timed out.";
		}
		else {
			Log << "Device probe completed.";
		}
		
		//pa_operation_unref(pa_op);

	}
	
	bool IsAvailable() {
		return pa_state == pa_ready;
	}

	///@endcond
}
