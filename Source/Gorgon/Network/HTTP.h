#pragma once

#include <optional>
#include <string>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "../Event.h"
#include "../String.h"  // for CaseInsensitiveLess

// curl type used in header so forward declare instead of including curl headers
struct curl_slist;  

namespace Gorgon :: Network {

    /**
    * This class provides HTTP functionality. Supports mainly non-blocking operations, 
    * but also has a blocking method for text retrieval. Non-blocking operations are executed
    * in a separate thread, but the events are fired in the main thread. You may use
    * Encoding::URI for encoding the URL if needed. Destructor waits for the current
    * operation to finish, so you can safely destroy the object while an operation is running.
    * HTTP objects should only be accessed from a single thread, ideally from the main
    * thread, as the events are fired in the main thread.
    *
    * Request headers can be configured globally via SetHeader/RemoveHeader/ClearHeaders.
    * Individual requests can override or add to these using the optional headers parameter.
    * Per-request headers are not persisted. POST requests are made by passing postData to
    * any of the request methods.
    *
    * Cookie storage can be enabled with StoreCookies(true). When enabled, Set-Cookie
    * response headers are automatically parsed and stored. Stored cookies are sent with
    * subsequent requests. Manual cookie management is available via SetCookie/GetCookie/etc.
    */
    class HTTP {
    public:
        /// Initializes HTTP networking system. It is called automatically
        static void Initialize();

        /**
        * Represents an HTTP error
        */
        class Error : public std::exception {
        public:

            /// Code of the error
            enum Code {
                /// No error occurs, you should not be getting this.
                NoError                    =  0,
                
                /// Given URL is malformed
                BadURL                    =  3,
                
                /// Cannot find the specified host
                HostResolutionFailed    =  6,
                
                /// Cannot connect to the specified host
                ConnectionFailed        =  7,
                
                /// Access denied
                AccessDenied            =  9,
                
                /// System run out of memory
                OutOfMemory                = 27,
                
                /// Cannot login to the given host
                LoginError                = 67,
                
                /// Page not found
                PageNotFound            =404,
                
                /// An unknown error has occurred
                Unknown                    = -1,
            };


            /// Constructor
            Error(const std::string &message="", Code error=NoError) : message(message), error(error) 
            { }

            virtual const char* what() const throw() override {
                return message.c_str();
            }
            
            /// Destructor
            virtual ~Error() throw() {}

            /// Error message text
            std::string message;
            
            /// Error code
            Code   error;
        };

        /**
        * Represents an HTTP cookie with optional attributes.
        */
        struct Cookie {
            /// Cookie name
            std::string name;
            /// Cookie value
            std::string value;
            /// Domain the cookie applies to
            std::string domain;
            /// Path the cookie applies to
            std::string path;
            /// Expiry date string (from Expires attribute)
            std::string expires;
            /// Max-Age in seconds (-1 means not set)
            int maxAge = -1;
            /// Whether the cookie should only be sent over HTTPS
            bool secure = false;
            /// Whether the cookie is inaccessible to JavaScript
            bool httpOnly = false;
            /// SameSite attribute value (empty if not set)
            std::string sameSite;

            /// Parses a Set-Cookie header value into a Cookie object.
            static Cookie Parse(const std::string &setCookieValue);

            /// Builds a Cookie header value string (name=value) for sending with requests.
            /// Only name and value are included; attributes are not sent to servers.
            std::string Build() const;
        };

        /// Storage type for headers (case-insensitive keys)
        using HeaderStorage = std::map<std::string, std::string, Gorgon::String::CaseInsensitiveLess>;
        /// Convenience alias used by request methods for per-request overrides
        using HeaderMap = std::map<std::string, std::string>;

        /// Tag type used to select the Get/Post overload that stores response
        /// data in a temporary internal vector passed to DataTransferCompletedEvent.
        struct DataTag {};

        /// Convenience tag instance for the temporary-vector Get/Post overloads.
        inline static constexpr DataTag ObtainData{};
    
        HTTP();
        ~HTTP();

        /// Requests a text from the URL. This function blocks the current thread and should
        /// not be preferred for text transfers over the internet as the UI will freeze while
        /// this function is running.
        static std::string BlockingGetText(const std::string &URL);

        /// Executed when GetText operation is completed.
        Event<HTTP, std::string&> TextTransferCompletedEvent;

        /// Fired when response headers are available (non-blocking operations only).
        /// The provided header storage contains header names and values (case-
        /// insensitive matching). The storage is owned by the HTTP instance and
        /// valid only during the event call; copy it if you need to keep it.
        Event<HTTP, HeaderStorage&> ResponseHeadersReceivedEvent;
        
        /// Fired when GetData operation is completed. The given vector is temporary, 
        /// it will be destroyed after used. You may swap its data if you need it
        Event<HTTP, std::vector<Byte>&> DataTransferCompletedEvent;
        
        /// Fired when GetFile operation is completed. 
        Event<HTTP> FileTransferCompletedEvent;
        
        /// Fired if an error occurs
        Event<HTTP, Error> TransferErrorEvent;

        // ---- Request header management ----

        /// Access to the global storage of request headers that will be sent
        /// with every request.
        const HeaderStorage& GetHeaders() const { return globalHeaders; }

        /// Return a single header value from global headers. Returns nullopt if not found.
        std::optional<std::string> GetHeader(const std::string &name) const {
            auto it = globalHeaders.find(name);
            if(it != globalHeaders.end())
                return it->second;
            return std::nullopt;
        }

        /// Sets a global request header that will be sent with every request.
        /// If the header already exists, its value is replaced.
        void SetHeader(const std::string &name, const std::string &value);

        /// Removes a global request header.
        void RemoveHeader(const std::string &name);

        /// Clears all global request headers.
        void ClearHeaders();

        /// Returns true if a global header is set on this HTTP object.
        bool HasHeader(const std::string &name) const { return globalHeaders.find(name) != globalHeaders.end(); }

        // ---- Cookie management ----

        /// Enables or disables automatic cookie storage. When enabled, Set-Cookie
        /// response headers are automatically parsed and stored. Stored cookies
        /// are sent with subsequent requests.
        void StoreCookies(bool state);

        /// Returns whether cookie storage is enabled.
        bool IsStoringCookies() const { return storecookies; }

        /// Clears all stored cookies.
        void ClearCookies();

        /// Returns all stored cookies.
        std::vector<Cookie> GetCookies() const;

        /// Returns the value of a stored cookie by name. Returns empty string if not found.
        std::string GetCookie(const std::string &name) const;

        /// Returns full cookie info by name. Returns a default Cookie if not found.
        Cookie GetCookieInfo(const std::string &name) const;

        /// Returns true if a cookie with the given name is stored.
        bool HasCookie(const std::string &name) const;

        /// Sets a cookie from a Cookie object.
        void SetCookie(const Cookie &cookie);

        /// Sets a cookie from a name and value pair.
        void SetCookie(const std::string &name, const std::string &value);

        // ---- GET requests ----

        /// Requests text data from the given URL (GET request).
        /// @param overrideHeaders Optional per-request headers (not persisted)
        void Get(const std::string &URL, const HeaderMap &overrideHeaders = {});

        /// Requests data from the given URL (GET request). Received data will be stored 
        /// in a temporary vector which will be passed to the event handler.
        void Get(const std::string &URL, DataTag, const HeaderMap &overrideHeaders = {});

        /// Requests data from the given URL (GET request). Received data will be stored 
        /// in the supplied vector. Ensure the vector lifetime is longer than the operation.
        void Get(const std::string &URL, std::vector<Byte> &vec, const HeaderMap &overrideHeaders = {});

        /// Downloads the given URL to the supplied filename (GET request).
        void Get(const std::string &URL, const std::string &filename, const HeaderMap &overrideHeaders = {});

        /// Streams the data to the given output stream (GET request).
        void Get(const std::string &URL, std::ostream &stream, const HeaderMap &overrideHeaders = {});

        // ---- POST requests ----

        /// Posts data and retrieves the response as text.
        /// @param postData Raw POST body (e.g. from HTTPQuery::Convert() or JSON)
        /// @param overrideHeaders Optional per-request headers (not persisted)
        void Post(const std::string &URL, const std::string &postData, const HeaderMap &overrideHeaders = {});

        /// Posts data and retrieves the response in a temporary vector.
        void Post(const std::string &URL, const std::string &postData, DataTag, const HeaderMap &overrideHeaders = {});

        /// Posts data and retrieves the response in the supplied vector.
        void Post(const std::string &URL, const std::string &postData, std::vector<Byte> &vec, const HeaderMap &overrideHeaders = {});

        /// Posts data and saves the response to a file.
        void Post(const std::string &URL, const std::string &postData, const std::string &filename, const HeaderMap &overrideHeaders = {});

        /// Posts data and streams the response to the given output stream.
        void Post(const std::string &URL, const std::string &postData, std::ostream &stream, const HeaderMap &overrideHeaders = {});

        /// Check if the process is still running
        bool IsRunning() const { return isrunning; }

        /// Access to the most recently received response headers. Valid only while
        /// `ResponseHeadersReceivedEvent` is being handled or immediately afterward
        /// (before the next request starts).
        const HeaderStorage& GetResponseHeaders() const { return responseHeaders; }

        /// Returns true if the given response header (from last request) was present.
        bool HasResponseHeader(const std::string &name) const { return responseHeaders.find(name) != responseHeaders.end(); }

        /// Returns the value of a response header from the last request. Returns nullopt if not found.
        std::optional<std::string> GetResponseHeader(const std::string &name) const {
            auto it = responseHeaders.find(name);
            if(it != responseHeaders.end())
                return it->second;
            return std::nullopt;
        }

        /// Should only be called if Gorgon main loop is not running.
        /// Otherwise it will be called automatically by the main.
        void onframe();

    private:
      void deletevec(std::vector<Byte> *vec);

      // callback invoked by libcurl for each header line received
      static size_t headerwriter(void *ptr, size_t size, size_t nmemb, void *userdata);

      void operation();

      // Builds a merged curl_slist from globalHeaders + overrideHeaders + cookies.
      // The returned list must be freed with curl_slist_free_all after perform.
      // Also updates lastRequestHeaders with the merged result.
      curl_slist *buildRequestHeaders(const HeaderMap &overrideHeaders);

      // Sets up common curl options, headers, and optional POST body. Must be called
      // while mtx is held (before spawning the thread). Returns the slist pointer
      // that must be freed after curl_easy_perform.
      void setupCurl(const std::string &URL, const HeaderMap &overrideHeaders,
                     const std::string &postData = "");

      void startOperation(const std::string &URL, std::ostream &stream,
                          const HeaderMap &overrideHeaders, const std::string &postData = "");
      void startOperation(const std::string &URL, std::vector<Byte> &vec,
                          const HeaderMap &overrideHeaders, const std::string &postData = "");
      void startTextOperation(const std::string &URL, const HeaderMap &overrideHeaders,
                              const std::string &postData = "");

      void *curl = nullptr;
      std::string tempstr;
      std::string tempPostData; // stored so it outlives the async operation
      std::unique_ptr<std::vector<Byte>,
                      std::function<void(std::vector<Byte> *)>>
          tempvec = {nullptr,
                     std::bind(&HTTP::deletevec, this, std::placeholders::_1)};
      std::ofstream tempfile;
      Error err;
      Event<>::Token token;

      std::mutex mtx;
      std::mutex headerMutex; // protects response headers map and availability flag
      std::thread runner;

      bool isrunning = false;
      inline static bool isinitialized = false;

      enum { None, Text, Data, OwnedData, File, Stream } currentoperation;

      // Response headers storage filled by headerwriter callback
      HeaderStorage responseHeaders;
      bool headersAvailable = false;

      // Pending Set-Cookie values collected by headerwriter (thread-safe via headerMutex)
      std::vector<std::string> pendingSetCookies;

      // Global request headers set by the user
      HeaderStorage globalHeaders;

      // Last request headers (merged global+override+cookie header)
      HeaderStorage lastRequestHeaders;

      // Cookie storage
      bool storecookies = false;
      std::map<std::string, Cookie> cookies;

      // curl_slist pointer for request headers, freed after operation completes
      void *requestHeaderList = nullptr;
    };
}

