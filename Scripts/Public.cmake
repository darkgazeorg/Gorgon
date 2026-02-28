
# --- RESOURCE COPY FUNCTIONS ---
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
        COMMAND ${CMAKE_COMMAND} -E remove_directory "$<TARGET_FILE_DIR:${target}>/${source_dir}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/${source_dir}"
            "$<TARGET_FILE_DIR:${target}>/${source_dir}"
        COMMENT "Syncing directory: ${source_dir}"
    )
    # Also ensure it gets installed
    install(DIRECTORY "${source_dir}" DESTINATION bin)
endfunction()

function(copy_commands_to_root TARGET_NAME)
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/compile_commands.json"
            "${CMAKE_SOURCE_DIR}/compile_commands.json"
        COMMENT "Copying compile_commands.json to source root for clangd..."
        VERBATIM
    )
endfunction()
