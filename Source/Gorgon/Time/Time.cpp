#include "../Time.h"
#include "Timer.h"

#include "../Types.h"
#include "../String.h"
#include <stdint.h>
#include <stdexcept>
#include <cmath>

//TODO uncomment
#include "../UI/Dialog.h"

namespace Gorgon { namespace Time {
	
	void Initialize() {
		internal::framestart=GetTime();
		internal::deltatime =0;
	}
	
	tm totm(const Date &d) {
		tm a = {};
		
		a.tm_sec   = d.Second;
		a.tm_min   = d.Minute;
		a.tm_hour  = d.Hour;
		a.tm_mday  = d.Day;
		a.tm_mon   = d.Month-1;
		a.tm_year  = d.Year-1900;
		a.tm_wday  = d.Weekday;
		
		return a;
	}
	
	void fromtm(Date &ret, tm timeinfo) {
		ret.Year 	   =(int)timeinfo.tm_year+1900;
		ret.Month	   =Date::MonthType(timeinfo.tm_mon+1);
		ret.Day		   =(int)timeinfo.tm_mday;
		ret.Hour 	   =(int)timeinfo.tm_hour;
		ret.Minute	   =(int)timeinfo.tm_min;
		ret.Second	   =(int)timeinfo.tm_sec;
		ret.Weekday	   =Date::WeekdayType(timeinfo.tm_wday);
	}
	
	Date::Date(time_t systemtime) {
        struct tm inf = *localtime(&systemtime);
        fromtm(*this, inf);
    }
    
    time_t Date::SystemTime() const {
        auto t = totm(*this);
        
        return mktime(&t);
    }

	
	bool Date::Parse(std::string isodate) {
		isodate=String::Trim(isodate);
		
		//!TODO Check for - and :
		try {
			Year   =String::To<int>(isodate.substr( 0, 4));
			Month  =MonthType(String::To<int>(isodate.substr( 5, 2)));
			Day    =String::To<int>(isodate.substr( 8, 2));
            
            if(isodate.length()>10) {
                Hour   =String::To<int>(isodate.substr(11, 2));
                Minute =String::To<int>(isodate.substr(14, 2));
                Second =String::To<int>(isodate.substr(17, 2));
            }
            else {
                Hour = Minute = Second = 0;
            }
            
			if(isodate.length()<=19) {
				Timezone=LocalTimezone();
			}
			else if(isodate[19]=='Z') {
				Timezone=0;
			}
			else {
				Timezone = String::To<int>(isodate.substr(19, 5));
				Timezone = (Timezone/100)*60 + Timezone%100;
			}
			
			Millisecond=0;
		}
		catch(...) {
			return false;
		}
		
		
		return DetermineWeekday();
	}
	
	bool Date::Load(std::istream &source) {
		uint16_t ushorttemp;		
		int16_t shorttemp;
		
		source.read((char*)&ushorttemp, 2);
		Year=ushorttemp;
		
		Month =MonthType(source.get());
		Day   =source.get();
		Hour  =source.get();
		Minute=source.get();
		Second=source.get();
		
		source.read((char*)&ushorttemp, 2);
		Millisecond=ushorttemp;
		
		source.read((char*)&shorttemp, 2);
		Timezone=shorttemp;
		
		Byte csource=source.get();
		Byte c=Year + Month + Day + Hour + Minute + Second + Millisecond + Timezone;
		
		if(csource!=c) {
			return false;
		}
		
		return DetermineWeekday();
	}
	
	bool Date::Save(std::ostream &target) {
		char byte;
		uint16_t ushorttemp;		
		int16_t shorttemp;
		
		ushorttemp=Year;
		target.write((char*)&ushorttemp, 2);
		
		byte=Month;
		target.write(&byte, 1);
		
		byte=Day;
		target.write(&byte, 1);
		
		byte=Hour;
		target.write(&byte, 1);
		
		byte=Minute;
		target.write(&byte, 1);
		
		byte=Second;
		target.write(&byte, 1);

		ushorttemp=Millisecond;
		target.write((char*)&ushorttemp, 2);
		
		shorttemp=Timezone;
		target.write((char*)&shorttemp, 2);
		
		byte=Year + Month + Day + Hour + Minute + Second + Millisecond + Timezone;
		target.write(&byte, 1);
		
		return target.good();
	}
	
	std::string Date::ISODate() const {
		using std::setw;
		
		std::stringstream ss;
		
		ss<<std::setfill('0');
		ss<<setw(4)<<Year <<"-"
			<<setw(2)<<Month<<"-"
			<<setw(2)<<Day;
			
		return ss.str();
	}
	
	std::string Date::ISODateTime(bool timezone) const {
		using std::setw;
		
		std::stringstream ss;
		
		ss<<std::setfill('0');
		ss<<setw(4)<<Year  <<"-"
			<<setw(2)<<Month <<"-"
			<<setw(2)<<Day   <<"T"
			<<setw(2)<<Hour  <<":"
			<<setw(2)<<Minute<<":"
			<<setw(2)<<Second;
			
        if(timezone) {
            if(Timezone>0) 
                ss<<"+"<<setw(2)<<(Timezone/60)<<setw(2)<<(Timezone%60);
            else if(Timezone<0)
                ss<<"-"<<setw(2)<<(-Timezone/60)<<setw(2)<<(-Timezone%60);
            else
                ss<<"Z";
        }
			
		return ss.str();
	}
	
	const std::string &Date::MonthName_En() const {
		static const std::string names[]={
			"[!]", "January", "February",
			"March", "April", "May",
			"June", "July", "August",
			"September", "October", "November",
			"December"
		};
		
#ifndef NDEBUG
		if(Month > Dec) {
			throw std::logic_error("Invalid month");
		}
#endif
		return names[Month];
	}
	
	const std::string &Date::ShortMonthName_En() const {
		static const std::string names[]={
			"[!]", "Jan", "Feb",
			"Mar", "Apr", "May",
			"Jun", "Jul", "Aug",
			"Sept", "Oct", "Nov",
			"Dec"
		};
		
#ifndef NDEBUG
		if(Month > Dec) {
			throw std::logic_error("Invalid month");
		}
#endif
		return names[Month];
	}
	
	const std::string &Date::WeekdayName_En() const {
		static const std::string names[]={
			"Sunday", "Monday", "Tuesday", "Wednesday",
			"Thursday", "Friday", "Saturday"
		};
		
#ifndef NDEBUG
		if((int)Weekday > (int)Saturday) {
			throw std::logic_error("Invalid weekday");
		}
#endif
		return names[Weekday];
	}
	
	const std::string &Date::ShortWeekdayName_En() const {
		static const std::string names[]={
			"Sun", "Mon", "Tue", "Wed",
			"Thu", "Fri", "Sat"
		};
		
#ifndef NDEBUG
		if((int)Month > (int)Saturday) {
			throw std::logic_error("Invalid weekday");
		}
#endif
		return names[Weekday];
	}
	
	std::string Date::Date_En() const {
		using std::setw;
		
		std::stringstream ss;
		ss<<Day<<" "<<MonthName_En()<<" "<<Year<<", "<<WeekdayName_En();
		
		return ss.str();
	}
	
	std::string Date::ShortDate_En() const {
		using std::setw;
		
		std::stringstream ss;
		ss<<Day<<" "<<ShortMonthName_En()<<" "<<Year;
		
		return ss.str();
	}	
	std::string Date::Time() const {
		using std::setw;
		
		std::stringstream ss;
		ss<<std::setfill('0');
		ss<<setw(2)<<Hour<<":"<<setw(2)<<Minute<<":"<<setw(2)<<Second;
		
		return ss.str();
	}
	
	std::string Date::ShortTime() const {
		using std::setw;
		
		std::stringstream ss;
		ss<<std::setfill('0');
		ss<<Hour<<":"<<setw(2)<<Minute;
		
		return ss.str();
	}
	
	std::string Date::Timezone_GMT() const {
		using std::setw;

		std::stringstream ss;
		ss<<"GMT";
		
		if(Timezone % 60 == 0) {
			if(Timezone>0)
				ss<<"+"<<(Timezone/60);
			else
				ss<<(Timezone/60);
		}
		else {
			if(Timezone>0)
				ss<<"+"<<(Timezone/60)<<":"<<setw(2)<<(Timezone%60);
			else
				ss<<"+"<<(-Timezone/60)<<":"<<setw(2)<<(-Timezone%60);
		}
		
		return ss.str();
	}
	
	int Date::LocalTimezone() {
		time_t time_local, time_gm;

		time(&time_local);
		time_gm    = mktime(gmtime(&time_local));

		double res=difftime(time_local, time_gm);

		
		
		return int(std::round(res/60));
	}
	
	bool Date::DetermineWeekday() {
		struct tm time = totm(*this);
		time_t t;
		
		if( (t=mktime(&time)) == -1) return false;
		
		
		
		Weekday=WeekdayType(time.tm_wday);
		
		return true;
	}

	double Date::operator - (const Date &other) {
		tm a = totm(*this), b = totm(other);
		
		time_t x = mktime(&a);
		time_t y = mktime(&b);
		
		return difftime(x, y);
	}
	
	bool Date::operator == (const Date &other) const{
        tm a = totm(*this), b = totm(other);
        
        time_t x = mktime(&a);
		time_t y = mktime(&b);
        
        return y == x;
    }
	
	void Date::AddYears(int years) {
		tm a = totm(*this);
		a.tm_year += years;
		
		mktime(&a);
		
		fromtm(*this, a);
	}
	
	void Date::AddMonths(int months) {
		tm a = totm(*this);
		a.tm_mon += months;
		
		mktime(&a);
		
		fromtm(*this, a);
	}
	
	void Date::AddDays(int days) {
		tm a = totm(*this);
		a.tm_mday += days;
		
		mktime(&a);
		
		fromtm(*this, a);
	}
	
	void Date::AddHours(int hours) {
		tm a = totm(*this);
		a.tm_hour += hours;
		
		mktime(&a);
		
		fromtm(*this, a);
	}
	
	void Date::AddMinutes(int minutes) {
		tm a = totm(*this);
		a.tm_min += minutes;
		
		mktime(&a);
		
		fromtm(*this, a);
	}
	
	void Date::AddSeconds(int seconds) {
		tm a = totm(*this);
		a.tm_sec += seconds;
		
		mktime(&a);
		
		fromtm(*this, a);
	}

	
	namespace internal {
		unsigned long framestart=0;
		unsigned long deltatime=0;
	}
	
	void Timer::ShowDialog(const std::string &name, const std::string &title) const { 
		std::stringstream ss;
		ss<<name<<": "<<passed;

#ifdef GORGON_UI_SUPPORT
		UI::ShowMessage(title, ss.str());
#else
		std::cout<<ss.str()<<std::endl;
#endif
	}	
} }

