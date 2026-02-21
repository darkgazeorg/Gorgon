cmake_minimum_required(VERSION 3.25)

# Replace GorgonApp with your application name
project(GorgonApp LANGUAGES CXX)

# --- 1. Source files ---
# You can add your source files here.
# You can list them manually (recommended) or use the GLOB approach below.
set(APP_SOURCES
    Source/Main.cpp
)

# --- ALTERNATIVE: GLOB APPROACH ---
# If you prefer to just drop files into the folder and have them "just work":
# file(GLOB_RECURSE APP_SOURCES "Source/*.cpp" "Source/*.h")
# Note: If you add a new file, you must re-run CMake so it "sees" it.

# --- 2. GORGON FRAMEWORK ---
find_package(Gorgon REQUIRED)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 3. HELPER FUNCTIONS ---
# This mirrors a single file to the build directory every time it changes. Supports
# an optional second argument to specify a relative destination subfolder (e.g., "Textures").
function(gorgon_copy_resource target source_file)
    # Default relative destination is empty (which means the root of the output directory)
    set(rel_dest "")
    
    # If a third argument is passed, use it as the relative destination path
    if(ARGC GREATER 2)
        set(rel_dest "${ARGV2}")
    endif()

    # Extract just the filename (e.g., "icon.png")
    get_filename_component(file_name "${source_file}" NAME)

    # Resolve the final build output and installation directories
    if(rel_dest STREQUAL "")
        set(full_out_dir "$<TARGET_FILE_DIR:${target}>")
        set(install_dest "bin")
    else()
        set(full_out_dir "$<TARGET_FILE_DIR:${target}>/${rel_dest}")
        set(install_dest "bin/${rel_dest}")
    endif()

    add_custom_command(TARGET ${target} POST_BUILD
        # Ensure the destination folder actually exists before copying
        COMMAND ${CMAKE_COMMAND} -E make_directory "${full_out_dir}"
        
        # Copy the file
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/${source_file}"
            "${full_out_dir}/${file_name}"
            
        COMMENT "Syncing resource: ${file_name} to ${install_dest}"
    )
    
    # Install to the correct subfolder inside 'bin'
    install(FILES "${source_file}" DESTINATION "${install_dest}")
endfunction()

# This mirrors an entire folder and removes "retired" (deleted) files.
function(gorgon_copy_directory target source_dir)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_FILE_DIR:${target}>/${source_dir}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/${source_dir}"
            "$<TARGET_FILE_DIR:${target}>/${source_dir}"
        COMMENT "Syncing directory: ${source_dir}"
    )
    # Also ensure it gets installed
    install(DIRECTORY "${source_dir}" DESTINATION bin)
endfunction()

# --- 4. THE EXECUTABLE ---
add_executable(${PROJECT_NAME} ${APP_SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE Gorgon::Gorgon)

# Set output to the local 'Bin' folder for easy access during development
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/Bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/Bin"
)

# --- 5. ASSET MANAGEMENT ---
# Example: Copy a single splash image and the entire textures folder.
# Add your own assets here!
# gorgon_copy_resource(${PROJECT_NAME} "Resources/logo.png")
# gorgon_copy_directory(${PROJECT_NAME} "Resources/Textures")

# --- 6. INSTALLATION & PACKAGING ---
include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME} 
    RUNTIME_DEPENDENCY_SET app_deps
    RUNTIME DESTINATION bin
)

install(RUNTIME_DEPENDENCY_SET app_deps
    DESTINATION bin
)

# CPack handles the creation of .zip, .tar.gz, or installers
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VENDOR "DarkGaze.Org") #Replace this
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_GENERATOR "ZIP;TGZ") # Generates a zip file for easy distribution
include(CPack)
