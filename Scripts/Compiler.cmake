
set(CMAKE_CXX_STANDARD 17)

target_compile_definitions(Gorgon PUBLIC UNICODE _UNICODE)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_compile_options(Gorgon PUBLIC 
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
    $<$<CXX_COMPILER_ID:MSVC>:/MP>
    $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
    $<$<CXX_COMPILER_ID:MSVC>:/wd4068>
)

target_compile_options(Gorgon PUBLIC
    # Enable warnings about missing return statements in non-void functions. 
    # This is a common source of bugs and should be treated as an error.
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Werror=return-type>
    $<$<CXX_COMPILER_ID:MSVC>:/we4715>
    # Enable warnings about uninitialized variables. 
    # This can help catch bugs where a variable is used before it has been assigned a value.
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Wuninitialized -Wparentheses>
    $<$<CXX_COMPILER_ID:MSVC>:/w44701 /w44703>
)
    
target_compile_options(Gorgon PRIVATE
    # Strict checks
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Wall -Wextra -Wpedantic -Wshadow -Werror>
    $<$<CXX_COMPILER_ID:MSVC>:/WX /W4>
)
