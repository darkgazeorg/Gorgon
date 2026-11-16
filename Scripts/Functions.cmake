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

# Compiles the given shaders to SPIR-V and embeds them in the output file. Result can be used the same
# way as target_embed_shaders: file names are variable names, and variables are strings containing the
# compiled shader. Vertex shaders should end with _V, others are assumed to be fragment shaders.
function(target_embed_spirv target_name output_file)
    find_program(GLSLC_EXECUTABLE glslc REQUIRED)

    set(current_output "${CMAKE_CURRENT_BINARY_DIR}/${output_file}")
    set(spirv_dir "${CMAKE_CURRENT_BINARY_DIR}/Spirv")

    set(absolute_inputs "")
    set(spirv_files "")
    foreach(shader IN LISTS ARGN)
        get_filename_component(shader_name "${shader}" NAME_WE)
        set(shader_input "${CMAKE_CURRENT_SOURCE_DIR}/${shader}")
        set(spirv_file "${spirv_dir}/${shader_name}.spv")

        if(shader_name MATCHES "_V$")
            set(shader_stage vertex)
        else()
            set(shader_stage fragment)
        endif()

        add_custom_command(
            OUTPUT "${spirv_file}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${spirv_dir}"
            COMMAND "${GLSLC_EXECUTABLE}" -fshader-stage=${shader_stage} -MD -MF "${spirv_file}.d" -o "${spirv_file}" "${shader_input}"
            DEPENDS "${shader_input}"
            DEPFILE "${spirv_file}.d"
            COMMENT "Compiling ${shader_name} to SPIR-V..."
            VERBATIM
        )

        list(APPEND absolute_inputs "${shader_input}")
        list(APPEND spirv_files "${spirv_file}")
    endforeach()

    add_custom_command(
        OUTPUT "${current_output}"
        COMMAND ShaderEmbedder --binary "${current_output}" ${spirv_files}
        DEPENDS ShaderEmbedder ${spirv_files}
        COMMENT "Generating ${output_file} from SPIR-V shaders..."
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
