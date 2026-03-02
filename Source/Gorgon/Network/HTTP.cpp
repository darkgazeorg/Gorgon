#include <cstring>
#include <iostream>
#include <stdexcept>

#include "../Utils/Assert.h"

#include "HTTP.h"
#include "../Main.h"

#define WINDOWS_LEAN_AND_MEAN
#include <curl/curl.h>

namespace Gorgon ::Network {

void HTTP::Initialize() {
    if(isinitialized)
        return;

    isinitialized = true;
    curl_global_init(CURL_GLOBAL_ALL);
}

static size_t stringwriter(void *ptr, size_t size, size_t nmemb, void *stream) {
    std::string &s = *(std::string *)stream;
    s += std::string((char *)ptr, size * nmemb);
    return size * nmemb;
}

static size_t streamwriter(void *ptr, size_t size, size_t nmemb, void *stream) {
    std::ostream &s = *(std::ostream *)stream;
    s.write((char *)ptr, size * nmemb);
    if(s.bad())
        return 0;
    else
        return size * nmemb;
}

static size_t vectorwriter(void *ptr, size_t size, size_t nmemb, void *vec) {
    std::vector<Byte> &s = *(std::vector<Byte> *)vec;

    auto prevsize = s.size();
    s.resize(s.size() + size * nmemb);
    std::memcpy(&s[prevsize], ptr, size * nmemb);

    return size * nmemb;
}

static HTTP::Error translateerror(CURLcode res) {
  HTTP::Error err;

  switch (res) {
  case CURLE_OK:
    break;
  case CURLE_UNSUPPORTED_PROTOCOL:
  case CURLE_URL_MALFORMAT:
    err = HTTP::Error("Bad URL", HTTP::Error::BadURL);
    break;
  case CURLE_COULDNT_RESOLVE_HOST:
    err = HTTP::Error("Cannot resolve host name",
                      HTTP::Error::HostResolutionFailed);
    break;
  case CURLE_COULDNT_CONNECT:
    err = HTTP::Error("Cannot connect to the host",
                      HTTP::Error::ConnectionFailed);
    break;
  case CURLE_REMOTE_ACCESS_DENIED:
    err = HTTP::Error("Access denied", HTTP::Error::AccessDenied);
    break;
  case CURLE_OUT_OF_MEMORY:
    err = HTTP::Error("Out of memory", HTTP::Error::OutOfMemory);
    break;
  case CURLE_LOGIN_DENIED:
    err = HTTP::Error("Login error", HTTP::Error::LoginError);
    break;
  case CURLE_HTTP_RETURNED_ERROR:
    err = HTTP::Error("Page not found", HTTP::Error::PageNotFound);
    break;
  default:
    err = HTTP::Error("Unknown error", HTTP::Error::Unknown);
    break;
  }

  return err;
}

HTTP::HTTP() : TextTransferCompletedEvent(this), 
               DataTransferCompletedEvent(this),
               FileTransferCompletedEvent(this), 
               TransferErrorEvent(this) 
{
  Initialize();

  curl = curl_easy_init();
  ASSERT(curl, "Cannot create curl handle. Initialization failed?");

  token = BeforeFrameEvent.Register(*this, &HTTP::onframe);
}

HTTP::~HTTP() { 
    BeforeFrameEvent.Unregister(token);

    if(runner.joinable())
        runner.join();

    curl_easy_cleanup(curl);
}

std::string HTTP::BlockingGetText(const std::string &URL) {
  CURL *curl_handle = curl_easy_init();

  Initialize();
  ASSERT(curl_handle, "Cannot create curl handle. Initialization failed?");

  std::string s;

  curl_easy_setopt(curl_handle, CURLOPT_URL, URL.c_str());
  curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, stringwriter);
  curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&s);
  curl_easy_setopt(curl_handle, CURLOPT_FAILONERROR, 1L);

  CURLcode res = curl_easy_perform(curl_handle);

  Error err = translateerror(res);

  curl_easy_cleanup(curl_handle);

  if(err.error)
    throw err;

  return s;
}

void HTTP::GetText(const std::string &URL) {
  tempstr = "";

  if(isrunning)
    throw std::runtime_error("Running another task at the moment.");

  isrunning = true;
  currentoperation = Text;

  curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
  curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, stringwriter);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&tempstr);
  curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

  // ifrunning is false, which it was to reach this point,
  // the thread is about to end, wait for it to finish
  if(runner.joinable())
    runner.join();

  runner = std::thread(&HTTP::operation, this);
}

void HTTP::GetFile(const std::string &URL, const std::string &filename) {
  tempfile.open(filename, std::ios::binary);

  if(isrunning)
    throw std::runtime_error("Running another task at the moment.");

  isrunning = true;
  currentoperation = File;

  stream(URL, tempfile);
}

void HTTP::GetStream(const std::string &URL, std::ostream &stream) {
  if(isrunning)
    throw std::runtime_error("Running another task at the moment.");

  isrunning = true;
  currentoperation = Stream;

  this->stream(URL, stream);
}

void HTTP::GetData(const std::string &URL) {
  if(isrunning)
    throw std::runtime_error("Running another task at the moment.");

  if(currentoperation == Data) {
    tempvec.reset();
  }

  isrunning = true;
  currentoperation = Data;

  tempvec.reset(new std::vector<Byte>);

  getdata(URL, *tempvec);
}

void HTTP::GetData(const std::string &URL, std::vector<Byte> &vec) {
  if(isrunning)
    throw std::runtime_error("Running another task at the moment.");

  if(currentoperation == Data) {
    tempvec.reset();
  }

  isrunning = true;
  currentoperation = OwnedData;

  tempvec.reset(&vec);

  getdata(URL, vec);
}

void HTTP::operation() {
  std::lock_guard<std::mutex> guard(mtx);
  CURLcode res = curl_easy_perform(curl);

  // should sync with main thread
  err = translateerror(res);
  isrunning = false;
}

void HTTP::onframe() {
  if(!mtx.try_lock())
      return;
    std::lock_guard<std::mutex> guard(mtx, std::adopt_lock);

    // if operation is set but not running this means the operation
    // has finished but the event has not been fired yet
    if(currentoperation != None && !isrunning) {
        if(err.error != 0) {
            TransferErrorEvent(err);
        }
        else {
            switch (currentoperation) {
            case Text:
                TextTransferCompletedEvent(tempstr);
                break;
            case Data:
                DataTransferCompletedEvent(*tempvec);
                // just in case if a new transfer is started during the event handler
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
            case None:
                break;
            }
        }

        if(!isrunning)
            currentoperation = None;
    }
}

void HTTP::getdata(const std::string &URL, std::vector<Byte> &vec) {
  curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
  curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, vectorwriter);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&vec);
  curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

  // ifrunning is false, the thread is about to end, wait for it to finish
  if(runner.joinable())
    runner.join();
  runner = std::thread(&HTTP::operation, this);
}

void HTTP::stream(const std::string &URL, std::ostream &stream) {
  curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
  curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &streamwriter);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&stream);
  curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

  // ifrunning is false, the thread is about to end, wait for it to finish
  if(runner.joinable())
    runner.join();
  runner = std::thread(&HTTP::operation, this);
}

void HTTP::deletevec(std::vector<Byte> *vec) {
  if(currentoperation != OwnedData) {
    delete vec;
  }
}

} // namespace Gorgon::Network
