# 1. Combine all sources and headers for grouping
set(ALL_GORGON_FILES ${GORGON_SOURCES} ${GORGON_HEADERS})

# 2. Filter files that live physically inside the Source directory
set(PHYSICAL_FILES ${ALL_GORGON_FILES})
list(FILTER PHYSICAL_FILES INCLUDE REGEX "^${PROJECT_SOURCE_DIR}/Source")

# 3. Filter everything else (Generated headers/shaders in the build folder)
set(GENERATED_FILES ${ALL_GORGON_FILES})
list(FILTER GENERATED_FILES EXCLUDE REGEX "^${PROJECT_SOURCE_DIR}/Source")

# 4. Apply the tree structure to physical files
if(PHYSICAL_FILES)
    source_group(TREE "${PROJECT_SOURCE_DIR}/Source" FILES ${PHYSICAL_FILES})
endif()

# 5. Group generated files together for easy access
if(GENERATED_FILES)
    source_group("Generated" FILES ${GENERATED_FILES})
endif()
