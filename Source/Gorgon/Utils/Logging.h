#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <mutex>

#include "../Time.h"
#include "../Utils/Console.h"

namespace Gorgon :: Utils {
	
	/**
	 * Eases logging procedure by appending necessary information to the given data and streams to a standard c++ stream.
	 * The log entries are prettified by indentation. The width of the output is determined for consoles, if not, 80
	 * will be used as default. The lines for streams will not be broken automatically, unless a SetWidth function is
	 * called.
	 */
	class Logger {
		friend class helper;
		std::mutex mtx;
		
		class helper {
			friend class Logger;
			
			Logger *parent;
			int shift = 0;
			bool extraenter = false;
			
			helper() : parent(nullptr) { }
			
			helper(Logger *parent, int shift, bool extraenter) : parent(parent), shift(shift), extraenter(extraenter) {
				if(!parent) return;
				parent->mtx.lock();
			}
			
			helper(helper &&h) {
				parent=h.parent;
				shift=h.shift;
				extraenter=h.extraenter;
				
				h.parent=nullptr;
			}
			
			helper &operator =(helper &&h) {
				if(parent)
					parent->mtx.unlock();
				
				parent=h.parent;
				shift=h.shift;
				extraenter=h.extraenter;
				
				h.parent=nullptr;
				
				return *this;
			}
			
		public:			
			~helper() {
				if(!parent) return;
					
				parent->mtx.unlock();
				
				if(extraenter)
					(*parent->stream) << "\n";
				
				(*parent->stream)<<std::endl;

                if(parent->IsColorFunctional())
                    parent->reset(*parent->stream);
            }
			
			using endl_t = decltype(&std::endl<char, std::char_traits<char>>);

			helper &operator <<(const endl_t &) {
				if(!parent) return *this;
				
				(*parent->stream) << std::endl << std::string(shift, ' ');
				
				extraenter=true;
				
#ifndef NDEBUG
				if(!(*parent->stream)) {
					throw std::runtime_error("Logging failed");
				}
#endif
				
				return *this;
			}
			
			template <class T_>
			helper &operator <<(const T_ &v) {
				if(!parent) return *this;
				
				(*parent->stream) << v;
				
#ifndef NDEBUG
				if(!(*parent->stream)) {
					throw std::runtime_error("Logging failed");
				}
#endif
				
				return *this;
			}
		};
		
	public:
        enum State {
            Message,
            Error,
            Notice,
            Success
        };
        
		/// Default constructor. Allows you to specify a section
		explicit Logger(const std::string &section = "", bool marktime = true, bool markdate = false) : 
			marktime(marktime), markdate(markdate), section(section)
		{ }

		explicit Logger(bool marktime, bool markdate = false) : Logger("", marktime, markdate) { }
		
		Logger(std::ostream &stream, const std::string &section = "", bool marktime = true, bool markdate = false) : 
			stream(&stream), marktime(marktime), markdate(markdate), section(section)
		{ 
			
		}

		~Logger() {
			CleanUp();
		}

		/// Initializes the logger to direct its input to the console.
		/// Only std::cout and std::cerr are supported. 
		void InitializeConsole(std::ostream &stream = std::cout) {
			CleanUp();
			
			this->stream=&stream;
			owner=false;

			if(stream.rdbuf() == std::cout.rdbuf()) {
            	console = StdConsole();
				hasconsole = true;
			}
			else if(stream.rdbuf() == std::cerr.rdbuf()) {
				console = StdErrorConsole();
				hasconsole = true;
			}
			else {
				hasconsole = false;
			}
		}

		/// Initializes the logger to direct its input to the given stream. 
		/// Ownership is not transferred
		void InitializeStream(std::ostream &stream) {
#ifndef NO_LOGGING
			CleanUp();
			this->stream=&stream;
			owner=false;
#endif
		}

		/// Opens and initializes the logger using the given filename. The file
		/// will automatically be closed when CleanUp is performed.
		void InitializeFile(const std::string &filename, bool append = false) {
#ifndef NO_LOGGING
			CleanUp();
			stream=new std::ofstream(filename, append ? std::ios::app : std::ios::out);
			owner=true;
#endif
		}

		/// Cleans the stream. If it is built by this object, it will be destroyed.
		/// After this point nothing will be logged.
		void CleanUp() {
			if(owner) {
				delete stream;
			}
			
			stream    = nullptr;
            console   = Console();
		}
		
		/// Sets the width to break lines from. Set to 0 to disable.
		void SetWidth(int width) {
			this->width = width;
		}
		
		/// Streams out the given value to the underlying stream. This function will automatically 
		/// add requested information in front. Always cascade entries. Every time a new `logger << ...`
		/// is called, header information will be printed out. You may use std::endl in your logs,
		/// but a new line will be added for all entries. An extra empty line will be inserted for
		/// multiline entries. Do not use "\n" as it will not be detected. Message state is used
		/// for this case.
		template <class T_>
		helper operator <<(const T_ &v) {
            return Log(v, Message);
        }
        
		/// Streams out the given value to the underlying stream. This function will automatically 
		/// add requested information in front. Always cascade entries. Every time a new `logger << ...`
		/// is called, header information will be printed out. You may use std::endl in your logs,
		/// but a new line will be added for all entries. An extra empty line will be inserted for
		/// multiline entries. Do not use "\n" as it will not be detected. ,
		/// @code
		/// logger.Log("Unexpected error: ", Utils::Logger::Error)<<ex.what();
		/// @endcode
        template <class T_>
        helper Log(const T_ &v, State state = Error) {
#ifndef NO_LOGGING
			if(!this->stream) return {nullptr, 0, false};
			auto &stream = *this->stream;
			
			int headw = marktime * 8 + markdate * 10 + (markdate && marktime) * 1 + ((markdate || marktime) && !section.empty()) * 1 + (int)section.size();
 			
			if(headw) headw += 2;

			helper h;
            
			if(width && headw*1.25 >= width) {
				stream<<std::endl;
				
				h = {this, 4, true};
			}
			else if(headw) {				
				h = {this, headw, false};
			}
			else {
				h = {this, 0, true};
			}
			
			if(marktime || markdate) {
				auto dt = Time::Date::Now();
				
				if(markdate) {
                    if(IsColorFunctional())
						console.SetColor(Console::Color::Cyan);
                        
                    stream<<dt.ISODate();
					
					if(marktime) stream<<" ";
				}
				
				if(marktime) {
					stream<<dt.Time();
				}
				
				if(!section.empty()) stream<<" ";
			}
			
            if(IsColorFunctional()) {
                reset(stream);
				console.SetBold(true);
                colorize(state);
            }
            
			if(!section.empty()) stream<<section;
			
			if(headw) stream<<"  ";

            if(IsColorFunctional())
                reset(stream);
            
            if(state != Message) {
                if(IsColorFunctional()) {
                    colorize(state);
                }
                else {
                    stream << (state == Error ? "Error! : " : "Success: ");
                }
            }
			
			return std::move(h << v);
#else
			return {nullptr, 0, false};
#endif
		}

		/// Sets the section of this logger.
		void SetSection(const std::string &value) {
            section = value;
        }
        
        /// Returns the current section of this logger.
        std::string GetSection() const {
            return section;
        }
        
        /// Sets whether to mark the time on log output
        void SetMarkTime(bool value) {
            marktime = value;
        }
        
        /// Returns whether time is being marked
        bool GetMarkTime() const {
            return marktime;
        }
        
        /// Sets whether to mark the date on log output
        void SetMarkDate(bool value) {
            markdate = value;
        }
        
        /// Returns whether date is being marked
        bool GetMarkDate() const {
            return markdate;
        }
        
        /// Enables color support, however, if the underlying stream does not allow coloring
        /// this will not have any effect. You may use ForceColor to output color coding to
        /// a device that does not support color.
        void EnableColor() {
            color = true;
        }
        
        
        /// Disable color output
        void DisableColor() {
            color = false;
        }
        
        /// Sets color enabled state. If color is enabled and the underlying stream does not 
        /// allow coloring setting coloring to true will not have any effect. You may use 
        /// ForceColor to output color coding to a device that does not support color.
        void SetColorEnabled(bool value) {
            color = value;
        }
        
        /// Whether color is enabled, a value of true is not a warranty that color output is
        /// working, use IsColorFunctional to make sure.
        bool IsColorEnabled() const {
            return color;
        }
        
        /// Returns whether the color output is currently working.
        bool IsColorFunctional() const {
            return color && hasconsole && console.IsColorSupported();
        }
        
        
	protected:
        
        void reset(std::ostream &stream) {
			if(console)
				console.Reset();
        }

		Console console;
        
		std::ostream *stream  = nullptr;
		bool owner            = false;
        
		bool marktime         = true;
		bool markdate         = true;
        
        bool hasconsole		  = false;
        bool color            = true;
		int  width            = 0;
		
		std::string section;
        
    private:
        void colorize(State state) {
            switch(state) {
                case Message: /*do nothing*/                            break;
                case Error:   console.SetColor(Console::Color::Red );   break;
                case Notice:  console.SetColor(Console::Color::Yellow); break;
                case Success: console.SetColor(Console::Color::Green);  break;
                default:      console.SetColor(Console::Color::White);  break;
            }
        }
	};

#ifdef NDEBUG
#	define DEBUGONLY(action) 
#else	
#	define DEBUGONLY(action) action
#endif

}
