#pragma once

#include <string>
#include <fstream>
#include <memory>
#include <mutex>
#include <thread>

#include "../Event.h"
#include "../Main.h"

//!Add info about Encoding::URI

namespace Gorgon :: Network {
	namespace HTTP {
		/// Initializes HTTP networking system. It is called automatically
		void Initialize();

		/// Requests a text from the URL. This function blocks the current thread and should
        /// not be preferred for text transfers over the internet as the UI will freeze while
        /// this fuınction is running.
		std::string BlockingGetText(const std::string &URL);

        /**
         * Represents an HTTP error
         */
		class Error : public std::exception {
		public:

            /// Code of the error
			enum Code {
                /// No error occurs, you should not be getting this.
				NoError					=  0,
                
                /// Given URL is malformed
				BadURL					=  3,
                
                /// Cannot find the specified host
				HostResolutionFailed	=  6,
                
                /// Cannot connect to the specified host
				ConnectionFailed		=  7,
                
                /// Access denied
				AccessDenied			=  9,
                
                /// System run out of memory
				OutOfMemory				= 27,
                
                /// Cannot login to the given host
				LoginError				= 67,
                
                /// Page not found
				PageNotFound			=404,
                
                /// An unknown error has occurred
				Unknown					= -1,
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
         * This class supports non-blocking HTTP operations. Events will occur on the thread
         * that calls NextFrame, thus it works as a synchronized event system.
         */
		class Nonblocking {
		public:

			Nonblocking();
			~Nonblocking() { 
				BeforeFrameEvent.Unregister(token);
			}

			/// Executed when GetText operation is completed.
			Event<Nonblocking, std::string&> TextTransferCompletedEvent;
			
			/// Fired when GetData operation is completed. The given vector is temporary, 
            /// it will be destroyed after used. You may swap its data if you need it
			Event<Nonblocking, std::vector<Byte>&> DataTransferCompletedEvent;
			
            /// Fired when GetFile operation is completed. 
			Event<Nonblocking> FileTransferCompletedEvent;
			
            /// Fired if an error occurs
			Event<Nonblocking, Error> TransferErrorEvent;

            /// Requests text data from the given URL
			void GetText(const std::string &URL);
            
            /// Requests data from the given URL
			void GetData(const std::string &URL);
            
            /// Requests data from the given URL. Received data will be stored in the supplied
            /// vector
			void GetData(const std::string &URL, std::vector<Byte> &vec);
            
            /// Downloads the given URL to the supplied filename
			void GetFile(const std::string &URL, const std::string &filename);
            
            /// Streams the data to the given output stream.
			void GetStream(const std::string &URL, std::ostream &stream);

            /// Check if the process is still running
			bool IsRunning() const { return isrunning; }

		private:
			void deletevec(std::vector<Byte> *vec) {
				if(currentoperation!=OwnedData) {
					delete vec;
				}
			}
			
			void operation();

			void stream(const std::string &URL, std::ostream &stream);
			void getdata(const std::string &URL, std::vector<Byte> &vec);
			
			void* curl;
			std::string tempstr;
			std::unique_ptr<std::vector<Byte>, std::function<void(std::vector<Byte>*)>> tempvec={nullptr, std::bind(&Nonblocking::deletevec, this, std::placeholders::_1)};
			std::ofstream tempfile;
			Error err;
			Event<>::Token token;
			
			std::mutex mtx;
			std::thread runner;
			
			bool isrunning = false;
			
			enum {
				None,
				Text,
				Data,
				OwnedData,
				File,
				Stream
			} currentoperation;
		};
	}

}

