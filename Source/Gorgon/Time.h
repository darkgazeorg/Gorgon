///@file Time.h contains time related functions and classes

#pragma once

#include <string>
#include <stdexcept>
#include <iostream>

namespace Gorgon {
	/// This module contains time related information.
	namespace Time  {
	
		/// Initializes Time module.
		void Initialize();
		
		class Date;
		
		/// Returns the current date. Should not be used for
		/// performance calculations, its not guaranteed to be precise.
		Date GetDate();
		
		/// This class represents a specific date including time information.
		/// Can be obtained using GetDateTime function. An Empty month marks the
		/// time as unset.
		class Date {
		public:
			/// Days of week. Starts from sunday
			enum WeekdayType {
				Sunday=0,
				Monday,
				Tuesday,
				Wednesday,
				Thursday,
				Friday,
				Saturday
			};
			
			/// Months, january is 1.
			enum MonthType {
				Empty=0,
				Jan = 1,
				Feb,
				Mar,
				Apr,
				May,
				Jun,
				Jul,
				Agu,
				Sep,
				Oct,
				Nov,
				Dec				
			};
			
			/// Default constructor, zero initializes the class, making it
			/// an unset time.
			Date() : Year(0), Month(Empty), Day(0), Hour(0), Minute(0),
			Second(0), Millisecond(0), Weekday(Sunday), Timezone(LocalTimezone())
			{ }
			
			/// Creates a new date object from the given ISO-8601 date string. @see Parse
			/// @throw std::runtime_error if the string is malformed or data is not valid
			Date(const std::string &isodate) { if(!Parse(isodate)) throw std::runtime_error("Invalid date"); }
			
			/// Reads a new date object from a binary stream. @see Save for details.
			/// @throw std::runtime_error if checksum is wrong or data is not valid
			explicit Date(std::istream &source) { if(!Load(source)) throw std::runtime_error("Invalid date"); }
			
			Date(time_t systemtime);
			
			/// Writes date object to a binary stream. Format is given below.
			/// numbers after names are number of bytes.
			/// @verbatim
			/// <year:2><month:1><day:1><hour:1><minute:1><millis:2><timezone:2><checksum:1>
			/// @endverbatim
			/// checksum is the sum of all numbers in base 256.
			/// @return true if successful. An empty date will be saved correctly.
			bool Save(std::ostream &target);
			
			/// Loads date object from a binary stream. @see Save for details.
			/// @return true if successful.
			bool Load(std::istream &source);
			
			/// Creates a new date object from the given ISO-8601 date string.
			/// Requires full information. Following format is recognized:
			///     [YYYY]-[MM]-[DD]T[HH]:[mm]:[SS](+/-)[TT][tt]
			/// * YYYY: 4 digit year
			/// * MM  : 2 digit month
			/// * DD  : 2 digit day
			/// * HH  : 2 digit hour
			/// * mm  : 2 digit minute
			/// * SS  : 2 digit second
			/// * TT  : 2 digit timezone hour
			/// * tt  : 2 digit timezone minute
			/// Timezone is optional, if omitted default is system timezone. It can also be
			/// specified as Z meaning UTC.
			/// @return true if successful.
			bool Parse(std::string isodate);
			
			/// ISO compliant date format. Contains only date
			std::string ISODate() const;
			
			/// ISO compliant date/time. This format should be used to serialize as text.
			std::string ISODateTime(bool timezone = true) const;
			
			/// Returns currently stored month's name in English. Month 0 is valid
			/// and returns "". 
			/// @throw std::logic_error (debug only) if month is invalid.
			const std::string &MonthName_En() const;
			
			/// Returns currently stored month's int name in English. Month 0 
			/// is valid and returns "". 
			/// @throw std::logic_error (debug only) if month is invalid.
			const std::string &ShortMonthName_En() const;
			
			/// Returns currently stored week day's name in English. 
			/// @throw std::logic_error (debug only) if weekday is invalid.
			const std::string &WeekdayName_En() const;
			
			/// Returns currently stored week day's int name in English. 
			/// @throw std::logic_error (debug only) if weekday is invalid.
			const std::string &ShortWeekdayName_En() const;
			
			/// Returns stored date in `day monthname year, weekday` format with
			/// English names for month and weekday.
			std::string Date_En() const;
			
			/// Returns stored date in `day intmonthname year` format with
			/// English names for month.
			std::string ShortDate_En() const;
			
			/// Returns stored time in `hour:minute:second` format.
			std::string Time() const;
			
			/// Returns stored time in `hour:minute` format.
			std::string ShortTime() const;
			
			/// Returns stored timezone. Format depends on the actual timezone.
			/// If the timezone offset contains only hours, the format is
			/// `GMT+H` or `GMT-H`. If there is minute offset, timezone is returned
			/// in `GMT+H:MM` or `GMT-H:MM` format. H is the hour without any
			/// leading zeroes, MM is the minute in two digit format.
			std::string Timezone_GMT() const;
			
			time_t SystemTime() const;
			
			/// Returns the system timezone in minutes. Might be negative.
			static int LocalTimezone();
			
			/// Adds specified amount of years to the date
			void AddYears(int years);
			
			/// Adds specified amount of months to the date
			void AddMonths(int months);
			
			/// Adds specified amount of days to the date
			void AddDays(int days);
			
			/// Adds specified amount of hours to the date
			void AddHours(int hours);
			
			/// Adds specified amount of minutes to the date
			void AddMinutes(int minutes);
			
			/// Adds specified amount of seconds to the date
			void AddSeconds(int seconds);
			
			/// Checks whether the stored time is actually set.
			bool IsSet() const {
				return Month==Empty;
			}
			
			/// Returns current time
			static Date Now() {
				return GetDate();
			}
			
			/// Gives the difference between two dates.
			double operator - (const Date &other);
			
			/// Compares 2 dates
            bool operator == (const Date &other) const;
            
            /// Unsets the stored time
			void Unset() {
				Year=0;
				Month=Empty;
				Day=0;
				Weekday=Sunday;
				Hour=0;
				Minute=0;
				Second=0;
				Millisecond=0;
				Timezone=0;
			}
			
			/// Determines the weekday from the stored date.
			/// @return true if successful
			bool DetermineWeekday();
			
			/// Full year
			unsigned int Year;
			
			/// Month starts from jan = 1
			MonthType Month;
			
			/// Day in month
			unsigned int Day;
			
			/// Hour in 24 hour format
			unsigned int Hour;
			
			/// Minute
			unsigned int Minute;
			
			/// Second
			unsigned int Second;
			
			/// This value is from the last second tick.
			unsigned int Millisecond;

			/// Day of the week, starts from sunday = 0
			WeekdayType Weekday;
			
			/// Timezone in minutes, can be negative. Note that some time zones
			/// has minute offset.
			int Timezone;
		};
		
        /// Output stream operator overload 
        inline std::ostream &operator << (std::ostream &out, const Date &date) {
            out<<date.ISODateTime(true);
            
            return out;
        }
        
		/// Returns current time in milliseconds. 
		/// @warning FrameStart should be used unless exact time is required.
		/// This function works slower and changes during a frame, which may
		/// cause synchronization issues.
		unsigned long GetTime();
		
		
		///@cond INTERNAL
		namespace internal {
			extern unsigned long framestart;
			extern unsigned long deltatime;
		}
		///@endcond

		
		/// Returns start time of the current frame in milliseconds. This function
		/// should be used for animations and effects as it is constant throughout
		/// a frame.
		inline unsigned long FrameStart() { 
			return internal::framestart; 
		}
		
		/// Returns the time passed since the last frame. This value is updated
		/// at the start of the frame. 
		inline unsigned long DeltaTime() {
			return internal::deltatime;
		}
		
		
	}
}
