# --- Windows Auto-Setup Bootstrap ---
if(GORGON_AUTO_SETUP AND WIN32 AND (NOT EXISTS "${CMAKE_SOURCE_DIR}/.tools" OR NOT EXISTS "${CMAKE_SOURCE_DIR}/.tools/vcpkg" OR NOT EXISTS "${CMAKE_SOURCE_DIR}/.tools/ninja"))
    message(STATUS "Tools directory (.tools) not found. Initializing auto-setup...")
    
    # Find PowerShell
    find_program(POWERSHELL_EXE NAMES powershell pwsh 
        HINTS $ENV{SystemRoot}/System32/WindowsPowerShell/v1.0)

    # Find Git (Looking in standard Windows install spots)
    find_program(GIT_EXECUTABLE NAMES git
        HINTS 
            "$ENV{ProgramFiles}/Git/cmd"
            "$ENV{ProgramFiles\(x86\)}/Git/cmd"
            "$ENV{LocalAppData}/Programs/Git/cmd"
    )

    if(POWERSHELL_EXE AND GIT_EXECUTABLE)
        message(STATUS "Using PowerShell: ${POWERSHELL_EXE}")
        message(STATUS "Found Git: ${GIT_EXECUTABLE}")

        execute_process(
            COMMAND "${POWERSHELL_EXE}" -ExecutionPolicy Bypass -File "${CMAKE_SOURCE_DIR}/setup.ps1" -GitPath "${GIT_EXECUTABLE}"
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            RESULT_VARIABLE SETUP_RESULT
        )
        
        if(NOT SETUP_RESULT EQUAL 0)
            message(FATAL_ERROR "Auto-setup failed. Check if Git/PowerShell are accessible.")
        endif()
    else()
        message(FATAL_ERROR "Required tools (PowerShell or Git) not found. Cannot automate setup.")
    endif()
endif()
