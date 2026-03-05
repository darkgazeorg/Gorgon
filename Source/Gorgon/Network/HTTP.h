#pragma once

#include <string>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <thread>

#include "../Event.h"

namespace Gorgon :: Network {

    /**
    * This class provides HTTP functionality. Supports mainly non-blocking operations, 
    * but also has a blocking method for text retrieval. Non-blocking operations are executed
    * in a separate thread, but the events are fired in the main thread. You may use
    * Encoding::URI for encoding the URL if needed. Destructor waits for the current
    * operation to finish, so you can safely destroy the object while an operation is running.
    * HTTP objects should only be accessed from a single thread, ideally from the main
    * thread, as the events are fired in the main thread.
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
    
        HTTP();
        ~HTTP();

        /// Requests a text from the URL. This function blocks the current thread and should
        /// not be preferred for text transfers over the internet as the UI will freeze while
        /// this fuınction is running.
        static std::string BlockingGetText(const std::string &URL);

        /// Executed when GetText operation is completed.
        Event<HTTP, std::string&> TextTransferCompletedEvent;

        /// Fired when request headers are available (non-blocking operations only).
        /// The provided map contains header names and values. The map is owned by
        /// the HTTP instance and valid only during the event call; copy it if you
        /// need to keep it.
        Event<HTTP, std::map<std::string,std::string>&> HeadersReceivedEvent;
        
        /// Fired when GetData operation is completed. The given vector is temporary, 
        /// it will be destroyed after used. You may swap its data if you need it
        Event<HTTP, std::vector<Byte>&> DataTransferCompletedEvent;
        
        /// Fired when GetFile operation is completed. 
        Event<HTTP> FileTransferCompletedEvent;
        
        /// Fired if an error occurs
        Event<HTTP, Error> TransferErrorEvent;

        /// Requests text data from the given URL
        void GetText(const std::string &URL);
        
        /// Requests data from the given URL. Received data will be stored in a temporary 
        /// vector which will be passed to the event handler. The vector will be destroyed
        /// after the event is fired, so you should not store a reference to it. You
        /// may swap its data if you need to keep it after the event is fired.
        void GetData(const std::string &URL);
        
        /// Requests data from the given URL. Received data will be stored in the supplied
        /// vector. Ensure the vector lifetime is longer than the operation. 
        void GetData(const std::string &URL, std::vector<Byte> &vec);
        
        /// Downloads the given URL to the supplied filename
        void GetFile(const std::string &URL, const std::string &filename);
        
        /// Streams the data to the given output stream.
        void GetStream(const std::string &URL, std::ostream &stream);

        /// Check if the process is still running
        bool IsRunning() const { return isrunning; }

        /// Access to the most recently received headers. Valid only while
        /// `HeadersReceivedEvent` is being handled or immediately afterward
        /// (before the next request starts).
        const std::map<std::string,std::string>& GetHeaders() const { return headers; }

        /// Should only be called if Gorgon main loop is not running.
        /// Otherwise it will be called automatically by the main.
        void onframe();

    private:
      void deletevec(std::vector<Byte> *vec);

      // callback invoked by libcurl for each header line received
      static size_t headerwriter(void *ptr, size_t size, size_t nmemb, void *userdata);

      void operation();

      void stream(const std::string &URL, std::ostream &stream);
      void getdata(const std::string &URL, std::vector<Byte> &vec);

      void *curl = nullptr;
      std::string tempstr;
      std::unique_ptr<std::vector<Byte>,
                      std::function<void(std::vector<Byte> *)>>
          tempvec = {nullptr,
                     std::bind(&HTTP::deletevec, this, std::placeholders::_1)};
      std::ofstream tempfile;
      Error err;
      Event<>::Token token;

      std::mutex mtx;
      std::mutex headerMutex; // protects headers map and availability flag
      std::thread runner;

      bool isrunning = false;
      inline static bool isinitialized = false;

      enum { None, Text, Data, OwnedData, File, Stream } currentoperation;

      // headers storage filled by headerwriter callback
      std::map<std::string,std::string> headers;
      bool headersAvailable = false;
    };
}

