if(DOCUMENTATION)
    find_package(Doxygen)

    if(DOXYGEN_FOUND)
        set(DOXY_INPUT "${PROJECT_SOURCE_DIR}/Source")
        set(DOXY_STRIP_PATH "${PROJECT_SOURCE_DIR}/Source")

        configure_file(
            "${PROJECT_SOURCE_DIR}/Scripts/Doxyfile.in" 
            "${PROJECT_BINARY_DIR}/Doxyfile" 
            @ONLY
        )
        
        doxygen_add_docs(docs
            ${GORGON_SOURCES}
            ALL # Built as part of the default target
            WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
            COMMENT "Generating API documentation with Doxygen"
        )

        if(BUILD_PDF)
            add_custom_target(pdf
                COMMAND ${CMAKE_MAKE_PROGRAM} -C "${CMAKE_BINARY_DIR}/latex"
                DEPENDS docs
                COMMENT "Building PDF documentation via LaTeX"
            )
        endif()

    else()
        message(WARNING "Doxygen not found. API documentation will not be generated.")
    endif()
endif()
