function(target_embed_shaders target_name output_file)
    set(current_output "${CMAKE_CURRENT_BINARY_DIR}/${output_file}")
    
    set(absolute_inputs "")
    foreach(shader IN LISTS ARGN)
        list(APPEND absolute_inputs "${CMAKE_CURRENT_SOURCE_DIR}/${shader}")
    endforeach()

    add_custom_command(
        OUTPUT "${current_output}"
        COMMAND ShaderEmbedder "${current_output}" ${absolute_inputs}
        DEPENDS ShaderEmbedder ${absolute_inputs}
        COMMENT "Generating ${output_file} from shaders..."
        VERBATIM
    )

    string(MAKE_C_IDENTIFIER "${output_file}" safe_suffix)
    set(sync_target "${target_name}_ShaderGen_${safe_suffix}")
    
    add_custom_target(${sync_target} DEPENDS "${current_output}")
    
    add_dependencies(${target_name} ${sync_target})

    target_include_directories(${target_name} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

    target_sources(${target_name} PRIVATE "${current_output}" ${absolute_inputs})
    set_source_files_properties("${current_output}" PROPERTIES GENERATED TRUE)
    set_source_files_properties(${absolute_inputs} PROPERTIES HEADER_FILE_ONLY TRUE)
endfunction()
