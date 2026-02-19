#include <stdexcept>
#include <cstring>
#include <iostream>

#include "../Utils/Assert.h"

#include "HTTP.h"


#define WINDOWS_LEAN_AND_MEAN
#include <curl/curl.h>


namespace Gorgon :: Network {

	/**
	 * Enables HTTP text, data and file transfer. Supports only GET operation. 
	 * 
	 * == Enabling ==
	 * HTTP module requires libcurl.
	 * 
	 * In Linux, installation of libcurl development libraries is enough. For fedora use "sudo dnf install libcurl-devel.i686".
	 * 
	 * In Windows, a compatible (7.45 is tested) libcurl source tree should be downloaded. In libcurl source tree,
	 * "projects\Windows\VC14\curl-all.sln" should be opened. There are two configurations that are required in a typical
	 * scenario LIB Debug and LIB Release. However, before compiling these configurations, there are two settings
	 * that needs to be changed in both configurations. From "PROJECT" -> "Properties" menu, go to 
	 * "Configuration Properties" -> "General" and change "Character Set" to "Not Set", in the same window, go to
	 * "C/C++" -> "Code Generation" and change library to "MT" or "MTd" for Relase and Debug builds respectively.
	 * After compiling libraries using these setting, from libcurl tree copy "build\Win32\VC14\LIB Debug\libcurld.lib",
	 * "build\Win32\VC14\LIB Release\libcurl.lib" and all files in "include\curl" to Gorgon tree Source/External/curl.
	 */
	namespace HTTP {

		static bool isinitialized = false;

		void Initialize() {
			if(isinitialized) return;
			curl_global_init(CURL_GLOBAL_ALL);
		}

		static size_t stringwriter(void *ptr, size_t size, size_t nmemb, void *stream) {
			std::string &s=*(std::string*)stream;
			s+=std::string((char*)ptr,size*nmemb);
			return size*nmemb;
		}

		static size_t streamwriter(void *ptr, size_t size, size_t nmemb, void *stream) {
			std::ostream &s=*(std::ostream*)stream;
			s.write((char*)ptr,size*nmemb);
			if(s.bad())
				return 0;
			else
				return size*nmemb;
		}

		static size_t vectorwriter(void *ptr, size_t size, size_t nmemb, void *vec) {
			std::vector<Byte> &s=*(std::vector<Byte>*)vec;
			
			auto prevsize=s.size();
			s.resize(s.size()+size*nmemb);
			std::memcpy(&s[prevsize], ptr, size*nmemb);

			return size*nmemb;
		}

		static Error translateerror(CURLcode res) {
			Error err;

			switch(res) {
			case CURLE_OK:
				break;
			case CURLE_UNSUPPORTED_PROTOCOL:
			case CURLE_URL_MALFORMAT:
				err=Error("Bad URL", Error::BadURL);
			case CURLE_COULDNT_RESOLVE_HOST:
				err=Error("Cannot resolve host name", Error::HostResolutionFailed);
			case CURLE_COULDNT_CONNECT:
				err=Error("Cannot connect to the host", Error::ConnectionFailed);
			case CURLE_REMOTE_ACCESS_DENIED:
				err=Error("Access denied", Error::AccessDenied);
			case CURLE_OUT_OF_MEMORY:
				err=Error("Out of memory", Error::OutOfMemory);
			case CURLE_LOGIN_DENIED:
				err=Error("Login error", Error::LoginError);
			case CURLE_HTTP_RETURNED_ERROR:
				err=Error("Page not found", Error::PageNotFound);
			default:
				err=Error("Unknown error", Error::Unknown);
			}

			return err;
		}

		std::string BlockingGetText(const std::string &URL) {
			CURL *curl_handle = curl_easy_init();

			Initialize();
			ASSERT(curl_handle, "Cannot create curl handle. Initialization failed?");

			std::string s;

			curl_easy_setopt(curl_handle, CURLOPT_URL, URL.c_str());
			curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
			curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, stringwriter);
			curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
			curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*)&s);
			curl_easy_setopt(curl_handle, CURLOPT_FAILONERROR, 1L);

			CURLcode res=curl_easy_perform(curl_handle);

			Error err=translateerror(res);

			if(err.error)
				throw err;

			curl_easy_cleanup(curl_handle);

			return s;
		}

		void Nonblocking::operation() {
			std::lock_guard<std::mutex> guard(mtx);	
			CURLcode res=curl_easy_perform(curl);

			//should sync with main thread
			err=translateerror(res);
			isrunning=false;
		}

		void Nonblocking::GetText(const std::string &URL) {
			tempstr="";

			if(isrunning) 
				throw std::runtime_error("Running another task at the moment.");

			isrunning=true;
			currentoperation=Text;


			curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, stringwriter);
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&tempstr);
			curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

			//if running is false, which it was to reach this point, 
			//the thread is about to end, wait for it to finish
			if(runner.joinable()) runner.join();
			runner=std::thread(&Nonblocking::operation, this);
		}

		void Nonblocking::GetFile(const std::string &URL, const std::string &filename) {
			tempfile.open(filename, std::ios::binary);

			if(isrunning)
				throw std::runtime_error("Running another task at the moment.");

			isrunning=true;
			currentoperation=File;

			stream(URL, tempfile);
		}

		void Nonblocking::GetStream(const std::string &URL, std::ostream &stream) {
			if(isrunning)
				throw std::runtime_error("Running another task at the moment.");

			isrunning=true;
			currentoperation=Stream;

			this->stream(URL, stream);
		}

		void Nonblocking::stream(const std::string &URL, std::ostream &stream) {

			curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &streamwriter);
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&stream);
			curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

			//if running is false, the thread is about to end, wait for it to finish
			if(runner.joinable()) runner.join();
			runner=std::thread(&Nonblocking::operation, this);
		}

		void Nonblocking::GetData(const std::string &URL) {
			if(isrunning) 
				throw std::runtime_error("Running another task at the moment.");

			if(currentoperation==Data) {
				tempvec.reset();
			}

			isrunning=true;
			currentoperation=Data;

			tempvec.reset(new std::vector<Byte>);

			getdata(URL, *tempvec);
		}
		
		void Nonblocking::GetData(const std::string &URL, std::vector<Byte> &vec) {
			if(isrunning) 
				throw std::runtime_error("Running another task at the moment.");

			if(currentoperation==Data) {
				tempvec.reset();
			}

			isrunning=true;
			currentoperation=OwnedData;

			tempvec.reset(&vec);

			getdata(URL, vec);
		}

		void Nonblocking::getdata(const std::string &URL, std::vector<Byte> &vec) {

			curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, vectorwriter);
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&vec);
			curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

			//if running is false, the thread is about to end, wait for it to finish
			if(runner.joinable()) runner.join();
			runner=std::thread(&Nonblocking::operation, this);
		}

		Nonblocking::Nonblocking() : isrunning(false),
			TextTransferCompletedEvent(this),
			DataTransferCompletedEvent(this),
			FileTransferCompletedEvent(this),
			TransferErrorEvent(this)
		{

			Initialize();
			
			curl=curl_easy_init();
			ASSERT(curl, "Cannot create curl handle. Initialization failed?");

			token=BeforeFrameEvent.Register([&]() {
				if(!mtx.try_lock()) return;
				std::lock_guard<std::mutex> guard(mtx, std::adopt_lock);
				
				if(currentoperation!=None && !isrunning) {
					
					if(err.error!=0) {
						TransferErrorEvent(err);
					}
					else {
						switch(currentoperation) {
						case Text:
							TextTransferCompletedEvent(tempstr);
							break;
						case Data:
							DataTransferCompletedEvent(*tempvec);
							//just in case if a new transfer is started during the event handler
							if(!isrunning) {
								tempvec.reset();
							}
							break;
						case OwnedData:
							DataTransferCompletedEvent(*tempvec);
							if(!isrunning) {
								tempvec.reset();
							}
							break;
						case File:
							tempfile.close();
							FileTransferCompletedEvent();
							break;
						case Stream:
							FileTransferCompletedEvent();
							break;
						}
					}
					if(!isrunning)
						currentoperation=None;
				}
			});
		}


	}


}
