SET(Local
	../Audio.h
	Basic.h
	Audio.cpp
	
	Controllers.h
	Controllers.cpp
	
	Environment.h
    Environment.cpp
)

IF(AUDIOLIB STREQUAL "PULSE")
	LIST(APPEND Local PulseAudio.cpp)
ENDIF()


IF(AUDIOLIB STREQUAL "WASAPI")
	LIST(APPEND Local WASAPI.cpp)
	LIST(APPEND Local WASAPI.inc.h)
ENDIF()
