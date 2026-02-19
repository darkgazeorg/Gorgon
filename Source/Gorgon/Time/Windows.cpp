#include "../Time.h"
#include <Windows.h>

namespace Gorgon :: Time {

	unsigned long GetTime() {
		return timeGetTime();
	}

	Date GetDate() {
		SYSTEMTIME timeinfo;
		GetLocalTime(&timeinfo);

		Date ret;
		ret.Year 	   =(int)timeinfo.wYear;
		ret.Month	   =Date::MonthType(timeinfo.wMonth);
		ret.Day		   =(int)timeinfo.wDay;
		ret.Hour 	   =(int)timeinfo.wHour;
		ret.Minute	   =(int)timeinfo.wMinute;
		ret.Second	   =(int)timeinfo.wSecond;
		ret.Millisecond=(int)timeinfo.wMilliseconds;
		ret.Weekday	   =Date::WeekdayType(timeinfo.wDayOfWeek);
		ret.Timezone   =Date::LocalTimezone();

		return ret;
	}

}
