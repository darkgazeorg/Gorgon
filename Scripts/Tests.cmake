cmake_minimum_required(VERSION 3.25)

set(TEST_ROOT "${PROJECT_SOURCE_DIR}/Testing")
set(TEST_SOURCE_DIR "${TEST_ROOT}/Source")
set(TEST_RUNTIME_DIR "${TEST_ROOT}/Runtime")

include("${TEST_ROOT}/tests.cmake")

file(MAKE_DIRECTORY "${TEST_RUNTIME_DIR}")

function(gorgon_add_test_executable name mode exclude_from_all)
    set(target_name "${mode}Test-${name}")
    
    if(exclude_from_all)
        add_executable(${target_name} EXCLUDE_FROM_ALL "${TEST_SOURCE_DIR}/${mode}/${name}.cpp")
    else()
        add_executable(${target_name} "${TEST_SOURCE_DIR}/${mode}/${name}.cpp")
    endif()
    
    target_link_libraries(${target_name} PRIVATE Gorgon::Gorgon Catch2::Catch2WithMain)
    target_include_directories(${target_name} PRIVATE "${TEST_SOURCE_DIR}")

    target_compile_definitions(${target_name} PRIVATE 
        TEST 
        "TESTDIR=\"${TEST_RUNTIME_DIR}\""
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND COVERAGE)
        target_compile_options(${target_name} PRIVATE -coverage)
        target_link_options(${target_name} PRIVATE -coverage)
    endif()

    set_target_properties(${target_name} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Testing/${mode}"
    )

    if(NOT WIN32)
        target_compile_definitions(${target_name} PRIVATE LINUX)
    endif()
endfunction()

# --- UNIT TESTS ---
if(UNIT_TESTS)
    include(FetchContent)
        FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.5.2
    )
    FetchContent_MakeAvailable(Catch2)

    foreach(test ${UnitTests})
        gorgon_add_test_executable(${test} "Unit" FALSE)
        
        # Register with CTest
        add_test(NAME ${test}
                 COMMAND $<TARGET_FILE:UnitTest-${test}>
                 WORKING_DIRECTORY "${TEST_RUNTIME_DIR}")
    endforeach()
endif()

# --- MANUAL TESTS ---
if(MANUAL_TESTS)
    add_custom_target(manualtest)

    foreach(test ${ManualTests})
        gorgon_add_test_executable(${test} "Manual" TRUE)
        
        add_dependencies(manualtest "ManualTest-${test}")
        
        add_custom_command(TARGET manualtest POST_BUILD
            COMMAND "ManualTest-${test}"
            WORKING_DIRECTORY "${TEST_RUNTIME_DIR}"
            COMMENT "Running Manual Test: ${test}"
        )
    endforeach()
endif()
