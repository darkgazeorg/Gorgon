#include "../Time.h"

#include <sys/time.h>
#include <time.h>
#include <stdlib.h>

namespace Gorgon :: Time {
	
	Date GetDate() {
		time_t rawtime;
        struct tm storage;
		struct tm *timeinfo = &storage;

		time ( &rawtime );
		timeinfo = localtime_r ( &rawtime , timeinfo);
			
		struct timeval  tv;
		gettimeofday(&tv, NULL);
		
		Date ret;
		ret.Year 	   =(int)timeinfo->tm_year+1900;
		ret.Month	   =Date::MonthType(timeinfo->tm_mon+1);
		ret.Day		   =(int)timeinfo->tm_mday;
		ret.Hour 	   =(int)timeinfo->tm_hour;
		ret.Minute	   =(int)timeinfo->tm_min;
		ret.Second	   =(int)timeinfo->tm_sec;
		ret.Millisecond=(int)tv.tv_usec/1000;
		ret.Weekday	   =Date::WeekdayType(timeinfo->tm_wday);
		ret.Timezone   =timeinfo->tm_gmtoff/60;
		
		return ret;
	}
		
	unsigned long GetTime() { 
		timeval tv;
		gettimeofday(&tv, 0 );
		return tv.tv_usec/1000+tv.tv_sec*1000;
	}

		
	
}
