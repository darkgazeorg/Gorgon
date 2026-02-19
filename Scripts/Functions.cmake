function(target_embed_shaders target_name output_file input_file)
    set(current_output "${CMAKE_CURRENT_BINARY_DIR}/${output_file}")
    set(current_input "${CMAKE_CURRENT_SOURCE_DIR}/${input_file}")

    add_custom_command(
        OUTPUT "${current_output}"
        COMMAND ShaderEmbedder "${current_output}" "${current_input}"
        DEPENDS ShaderEmbedder "${current_input}"
        COMMENT "Embedding shader ${input_file}"
        VERBATIM
    )

    target_sources(${target_name} PRIVATE "${current_output}")

    target_sources(${target_name} PRIVATE "${current_input}")
    set_source_files_properties("${current_input}" PROPERTIES HEADER_FILE_ONLY TRUE)
endfunction()
