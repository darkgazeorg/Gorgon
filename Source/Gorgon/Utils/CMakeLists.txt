target_sources(Gorgon
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/Source"
    FILES
        Console.h
		Assert.h
        Compiler.h
		Logging.h
		RefCounter.h
		ScopeGuard.h
)

if(WIN32)
    target_sources(Gorgon
        PRIVATE
            Assert_Win32.cpp
            Console_Win32.cpp
    )
else()
    target_sources(Gorgon
        PRIVATE
            Assert_Linux.cpp
            Console_Nix.cpp
    )
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
    target_sources(Gorgon
        PRIVATE
            Compiler_GCC.cpp
    )
elseif(MSVC)
    target_sources(Gorgon
        PRIVATE
            Compiler_MSVC.cpp
    )
endif()
