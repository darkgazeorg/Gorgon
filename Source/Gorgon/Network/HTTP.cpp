#include <cstring>
#include <iostream>
#include <stdexcept>
#include <algorithm>

#include "../Utils/Assert.h"

#include "HTTP.h"
#include "../Main.h"
#include "Gorgon/String.h"

#define WINDOWS_LEAN_AND_MEAN
#include <curl/curl.h>

namespace Gorgon ::Network {

// ---- Static helpers ----

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

// ---- Cookie parsing / building ----

HTTP::Cookie HTTP::Cookie::Parse(const std::string &setCookieValue) {
    Cookie c;
    // Split on semicolons
    std::string remaining = setCookieValue;
    bool first = true;
    while(!remaining.empty()) {
        std::string part;
        auto semi = remaining.find(';');
        if(semi != std::string::npos) {
            part = remaining.substr(0, semi);
            remaining = remaining.substr(semi + 1);
        } else {
            part = remaining;
            remaining.clear();
        }
        part = String::Trim(part);
        if(part.empty()) continue;

        if(first) {
            // First part is name=value
            auto eq = part.find('=');
            if(eq != std::string::npos) {
                c.name = String::Trim(part.substr(0, eq));
                c.value = String::Trim(part.substr(eq + 1));
            } else {
                c.name = String::Trim(part);
            }
            first = false;
            continue;
        }

        // Attribute
        auto eq = part.find('=');
        std::string attrName, attrValue;
        if(eq != std::string::npos) {
            attrName = String::Trim(part.substr(0, eq));
            attrValue = String::Trim(part.substr(eq + 1));
        } else {
            attrName = String::Trim(part);
        }

        // Case-insensitive attribute matching
        std::string lower = String::ToLower(attrName);

        if(lower == "domain") c.domain = attrValue;
        else if(lower == "path") c.path = attrValue;
        else if(lower == "expires") c.expires = attrValue;
        else if(lower == "max-age") {
            try { c.maxAge = std::stoi(attrValue); } catch(...) {}
        }
        else if(lower == "secure") c.secure = true;
        else if(lower == "httponly") c.httpOnly = true;
        else if(lower == "samesite") c.sameSite = attrValue;
    }
    return c;
}

std::string HTTP::Cookie::Build() const {
    return name + "=" + value;
}

// ---- Response header callback ----

size_t HTTP::headerwriter(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    HTTP *self = (HTTP *)userdata;
    std::string header((char *)ptr, size * nmemb);

    // protect response headers with its own mutex to avoid locking mtx (which
    // is held by operation()) and causing a deadlock when the callback is
    // invoked during curl_easy_perform.
    std::lock_guard<std::mutex> guard(self->headerMutex);

    // empty line indicates end of headers
    if(header == "\r\n" || header == "\n") {
        self->headersAvailable = true;
        return size * nmemb;
    }

    auto colon = header.find(':');
    if(colon != std::string::npos) {
        std::string name = header.substr(0, colon);
        std::string value = header.substr(colon + 1);

        // trim whitespace and trailing CRLF
        while(!value.empty() && (value.back() == '\r' || value.back() == '\n'))
            value.pop_back();
        while(!value.empty() && value.front() == ' ')
            value.erase(value.begin());

        self->responseHeaders[name] = value;

        // Collect Set-Cookie headers for cookie storage
        if(String::ToLower(name) == "set-cookie") {
            self->pendingSetCookies.push_back(value);
        }
    }

    return size * nmemb;
}

// ---- Error translation ----

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

// ---- Constructor / Destructor ----

HTTP::HTTP() : TextTransferCompletedEvent(this), 
               ResponseHeadersReceivedEvent(this),
               DataTransferCompletedEvent(this),
               FileTransferCompletedEvent(this), 
               TransferErrorEvent(this) 
{
  Initialize();

  curl = curl_easy_init();
  ASSERT(curl, "Cannot create curl handle. Initialization failed?");

  // register callbacks that are common across operations
  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &HTTP::headerwriter);
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, this);

  token = BeforeFrameEvent.Register(*this, &HTTP::onframe);
}

HTTP::~HTTP() { 
    BeforeFrameEvent.Unregister(token);

    if(runner.joinable())
        runner.join();

    curl_easy_cleanup(curl);
}

// ---- Request header management ----

void HTTP::SetHeader(const std::string &name, const std::string &value) {
    globalHeaders[name] = value;
}

void HTTP::RemoveHeader(const std::string &name) {
    globalHeaders.erase(name);
}

void HTTP::ClearHeaders() {
    globalHeaders.clear();
}

// ---- Cookie management ----

void HTTP::StoreCookies(bool state) {
    storecookies = state;
}

void HTTP::ClearCookies() {
    cookies.clear();
}

std::vector<HTTP::Cookie> HTTP::GetCookies() const {
    std::vector<Cookie> result;
    result.reserve(cookies.size());
    for(auto &[name, cookie] : cookies)
        result.push_back(cookie);
    return result;
}

std::string HTTP::GetCookie(const std::string &name) const {
    auto it = cookies.find(name);
    if(it != cookies.end())
        return it->second.value;
    return "";
}

HTTP::Cookie HTTP::GetCookieInfo(const std::string &name) const {
    auto it = cookies.find(name);
    if(it != cookies.end())
        return it->second;
    return Cookie{};
}

bool HTTP::HasCookie(const std::string &name) const {
    return cookies.find(name) != cookies.end();
}

void HTTP::SetCookie(const Cookie &cookie) {
    cookies[cookie.name] = cookie;
}

void HTTP::SetCookie(const std::string &name, const std::string &value) {
    Cookie c;
    c.name = name;
    c.value = value;
    cookies[name] = c;
}

// ---- Curl setup helpers ----

curl_slist *HTTP::buildRequestHeaders(const HeaderMap &overrideHeaders) {
    curl_slist *headerList = nullptr;

    // Start with global headers (HeaderStorage)
    HeaderStorage merged = globalHeaders;

    // Override/add with per-request headers
    for(auto &[name, value] : overrideHeaders)
        merged[name] = value;

    // Build cookie header from stored cookies
    if(!cookies.empty()) {
        std::string cookieStr;
        for(auto &[name, cookie] : cookies) {
            if(!cookieStr.empty()) cookieStr += "; ";
            cookieStr += cookie.Build();
        }
        // Only set if user hasn't explicitly overridden Cookie header
        if(merged.find("Cookie") == merged.end())
            merged["Cookie"] = cookieStr;
    }

    // Store the headers we will actually send
    lastRequestHeaders = merged;

    // Convert to curl_slist
    for(auto &[name, value] : merged) {
        std::string line = name + ": " + value;
        headerList = curl_slist_append(headerList, line.c_str());
    }

    return headerList;
}

void HTTP::setupCurl(const std::string &URL, const HeaderMap &overrideHeaders,
                     const std::string &postData) {
    curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

    // Set request headers
    requestHeaderList = buildRequestHeaders(overrideHeaders);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, (curl_slist *)requestHeaderList);

    // POST vs GET
    if(!postData.empty()) {
        tempPostData = postData;
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, tempPostData.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)tempPostData.size());
    } else {
        tempPostData.clear();
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    }
}

// ---- Blocking operations ----

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

// ---- Shared operation runner ----

void HTTP::operation() {
  std::lock_guard<std::mutex> guard(mtx);
  CURLcode res = curl_easy_perform(curl);

  // Free the request header list now that perform is done
  if(requestHeaderList) {
    curl_slist_free_all((curl_slist *)requestHeaderList);
    requestHeaderList = nullptr;
  }
  // Reset POST state so next request doesn't accidentally POST
  curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, nullptr);

  // should sync with main thread
  err = translateerror(res);
  isrunning = false;
}

// ---- Start operation helpers ----

void HTTP::startTextOperation(const std::string &URL, const HeaderMap &overrideHeaders,
                              const std::string &postData) {
  tempstr = "";

  std::lock_guard<std::mutex> guard(mtx);
  if(isrunning)
    throw std::runtime_error("Running another task at the moment.");

  responseHeaders.clear();
  pendingSetCookies.clear();
  headersAvailable = false;

  isrunning = true;
  currentoperation = Text;

  setupCurl(URL, overrideHeaders, postData);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, stringwriter);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&tempstr);

  if(runner.joinable())
    runner.join();

  runner = std::thread(&HTTP::operation, this);
}

void HTTP::startOperation(const std::string &URL, std::vector<Byte> &vec,
                          const HeaderMap &overrideHeaders, const std::string &postData) {
  setupCurl(URL, overrideHeaders, postData);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, vectorwriter);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&vec);

  if(runner.joinable())
    runner.join();
  runner = std::thread(&HTTP::operation, this);
}

void HTTP::startOperation(const std::string &URL, std::ostream &stream,
                          const HeaderMap &overrideHeaders, const std::string &postData) {
  setupCurl(URL, overrideHeaders, postData);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &streamwriter);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&stream);

  if(runner.joinable())
    runner.join();
  runner = std::thread(&HTTP::operation, this);
}

// ---- GET requests ----

void HTTP::GetText(const std::string &URL, const HeaderMap &overrideHeaders) {
  startTextOperation(URL, overrideHeaders);
}

void HTTP::GetData(const std::string &URL, const HeaderMap &overrideHeaders) {
  std::lock_guard<std::mutex> guard(mtx);
  if(isrunning)
    throw std::runtime_error("Running another task at the moment.");

  if(currentoperation == Data)
    tempvec.reset();

  responseHeaders.clear();
  pendingSetCookies.clear();
  headersAvailable = false;

  isrunning = true;
  currentoperation = Data;

  tempvec.reset(new std::vector<Byte>);

  startOperation(URL, *tempvec, overrideHeaders);
}

void HTTP::GetData(const std::string &URL, std::vector<Byte> &vec, const HeaderMap &overrideHeaders) {
  std::lock_guard<std::mutex> guard(mtx);
  if(isrunning)
    throw std::runtime_error("Running another task at the moment.");

  if(currentoperation == Data)
    tempvec.reset();

  responseHeaders.clear();
  pendingSetCookies.clear();
  headersAvailable = false;

  isrunning = true;
  currentoperation = OwnedData;

  tempvec.reset(&vec);

  startOperation(URL, vec, overrideHeaders);
}

void HTTP::GetFile(const std::string &URL, const std::string &filename, const HeaderMap &overrideHeaders) {
  tempfile.open(filename, std::ios::binary);

  std::lock_guard<std::mutex> guard(mtx);
  if(isrunning)
    throw std::runtime_error("Running another task at the moment.");

  responseHeaders.clear();
  pendingSetCookies.clear();
  headersAvailable = false;

  isrunning = true;
  currentoperation = File;

  startOperation(URL, tempfile, overrideHeaders);
}

void HTTP::GetStream(const std::string &URL, std::ostream &stream, const HeaderMap &overrideHeaders) {
  std::lock_guard<std::mutex> guard(mtx);
  if(isrunning)
    throw std::runtime_error("Running another task at the moment.");

  responseHeaders.clear();
  pendingSetCookies.clear();
  headersAvailable = false;

  isrunning = true;
  currentoperation = Stream;

  startOperation(URL, stream, overrideHeaders);
}

// ---- POST requests ----

void HTTP::PostText(const std::string &URL, const std::string &postData, const HeaderMap &overrideHeaders) {
  startTextOperation(URL, overrideHeaders, postData);
}

void HTTP::PostData(const std::string &URL, const std::string &postData, const HeaderMap &overrideHeaders) {
  std::lock_guard<std::mutex> guard(mtx);
  if(isrunning)
    throw std::runtime_error("Running another task at the moment.");

  if(currentoperation == Data)
    tempvec.reset();

  responseHeaders.clear();
  pendingSetCookies.clear();
  headersAvailable = false;

  isrunning = true;
  currentoperation = Data;

  tempvec.reset(new std::vector<Byte>);

  startOperation(URL, *tempvec, overrideHeaders, postData);
}

void HTTP::PostData(const std::string &URL, const std::string &postData,
                    std::vector<Byte> &vec, const HeaderMap &overrideHeaders) {
  std::lock_guard<std::mutex> guard(mtx);
  if(isrunning)
    throw std::runtime_error("Running another task at the moment.");

  if(currentoperation == Data)
    tempvec.reset();

  responseHeaders.clear();
  pendingSetCookies.clear();
  headersAvailable = false;

  isrunning = true;
  currentoperation = OwnedData;

  tempvec.reset(&vec);

  startOperation(URL, vec, overrideHeaders, postData);
}

void HTTP::PostFile(const std::string &URL, const std::string &postData,
                    const std::string &filename, const HeaderMap &overrideHeaders) {
  tempfile.open(filename, std::ios::binary);

  std::lock_guard<std::mutex> guard(mtx);
  if(isrunning)
    throw std::runtime_error("Running another task at the moment.");

  responseHeaders.clear();
  pendingSetCookies.clear();
  headersAvailable = false;

  isrunning = true;
  currentoperation = File;

  startOperation(URL, tempfile, overrideHeaders, postData);
}

void HTTP::PostStream(const std::string &URL, const std::string &postData,
                      std::ostream &stream, const HeaderMap &overrideHeaders) {
  std::lock_guard<std::mutex> guard(mtx);
  if(isrunning)
    throw std::runtime_error("Running another task at the moment.");

  responseHeaders.clear();
  pendingSetCookies.clear();
  headersAvailable = false;

  isrunning = true;
  currentoperation = Stream;

  startOperation(URL, stream, overrideHeaders, postData);
}

// ---- Frame handler ----

void HTTP::onframe() {
    // handle response headers first using header mutex only; this can run even
    // while the operation thread is performing a request since it doesn't lock mtx.
    {
        std::lock_guard<std::mutex> hguard(headerMutex);
        if(headersAvailable) {
            // Process pending cookies before firing the event
            if(storecookies) {
                for(auto &raw : pendingSetCookies) {
                    Cookie c = Cookie::Parse(raw);
                    if(!c.name.empty())
                        cookies[c.name] = c;
                }
            }
            pendingSetCookies.clear();

            ResponseHeadersReceivedEvent(responseHeaders);
            headersAvailable = false;
            // keep response headers map around until cleared by next request
        }
    }

    if(!mtx.try_lock())
        return;
      
    std::lock_guard<std::mutex> guard(mtx, std::adopt_lock);

    // if operation is set but not running this means the operation
    // has finished but the event has not been fired yet
    if(currentoperation != None && !isrunning) {
        // Process any remaining pending cookies (in case headers arrived
        // after the last onframe but before operation finished)
        {
            std::lock_guard<std::mutex> hguard(headerMutex);
            if(storecookies) {
                for(auto &raw : pendingSetCookies) {
                    Cookie c = Cookie::Parse(raw);
                    if(!c.name.empty())
                        cookies[c.name] = c;
                }
            }
            pendingSetCookies.clear();
        }

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

void HTTP::deletevec(std::vector<Byte> *vec) {
  if(currentoperation != OwnedData) {
    delete vec;
  }
}

} // namespace Gorgon::Network
