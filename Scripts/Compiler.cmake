target_compile_definitions(Gorgon PUBLIC UNICODE _UNICODE)

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
    # Strict checks with unknown pragmas explicitly ignored
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>: 
        -Wall -Werror -Wextra
        -Wno-unknown-pragmas -Wno-unused-parameter
    >
    $<$<CXX_COMPILER_ID:MSVC>: /W3 /wd4068 /wd4100 /wd4458>
)

if(WIN32)
    target_compile_definitions(Gorgon PUBLIC WIN32)
else()
    target_compile_definitions(Gorgon PUBLIC LINUX)
endif()
