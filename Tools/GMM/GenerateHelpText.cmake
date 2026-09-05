# GenerateHelpText.cmake
# Reads GMM.md, strips the Doxygen wrapper, and outputs a C++ header
# with the content as a raw string literal.

file(READ "${INPUT_FILE}" content)

# Strip the Doxygen page wrapper: /*! \page ... \n and trailing */
string(REGEX REPLACE "^/\\*![ \t]*\\\\page[^\n]*\n" "" content "${content}")
string(REGEX REPLACE "\n\\*/[ \t\r\n]*$" "" content "${content}")

# Write as a C++ raw string literal
file(WRITE "${OUTPUT_FILE}" 
    "#pragma once\n"
    "// Auto-generated from ${INPUT_FILE} - do not edit manually\n"
    "inline const char *GMM_HELP_TEXT = R\"gmm_help(\n"
    "${content}"
    "\n)gmm_help\";\n"
)
