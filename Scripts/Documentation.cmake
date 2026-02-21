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

        # warning-related variables default to off; documentation mode will enable them
        set(DOXYGEN_WARN_NO_PARAMDOC NO)

        if(DOCUMENTATION_MODE)
            # Strict mode: Show all warnings and progress
            set(DOXYGEN_QUIET NO)
            set(DOXYGEN_WARNINGS YES)
            set(DOXYGEN_WARN_IF_UNDOCUMENTED YES)
            set(DOXYGEN_WARN_IF_DOC_ERROR YES)
            set(DOXYGEN_WARN_IF_INCOMPLETE_DOC YES)
            set(DOXY_WARNINGS YES)
        else()
            # Silent mode: Hide progress and all warnings
            set(DOXYGEN_QUIET YES)
            set(DOXYGEN_WARNINGS NO)
            set(DOXYGEN_WARN_IF_UNDOCUMENTED NO)
            set(DOXYGEN_WARN_IF_DOC_ERROR NO)
            set(DOXYGEN_WARN_IF_INCOMPLETE_DOC NO)
            set(DOXYGEN_WARN_NO_PARAMDOC NO)
            set(DOXY_WARNINGS NO)
        endif()
        
        # force output into the source Docs folder rather than the build tree.
        # the HTML output will appear under <output>/html.
        set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_DOCUMENT_OUTPUT_DIRECTORY}")
        # break up the generated files into many subdirectories so explorers stay fast
        set(DOXYGEN_CREATE_SUBDIRS YES)
        set(DOXYGEN_CREATE_SUBDIRS_LEVEL 8)

        doxygen_add_docs(docs
            ${GORGON_HEADERS}
            ALL
            WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
            COMMENT "Generating API documentation with Doxygen"
        )

        # to make it easy to open the docs we write a tiny wrapper at
        # ${CMAKE_DOCUMENT_OUTPUT_DIRECTORY}/index.html that simply redirects
        # to the real HTML generated beneath the "html" subdirectory.  Using a
        # redirect avoids any broken paths for the javascript and css resources
        # which are referenced relative to the page itself.
        file(MAKE_DIRECTORY "${CMAKE_DOCUMENT_OUTPUT_DIRECTORY}")
        file(WRITE "${CMAKE_DOCUMENT_OUTPUT_DIRECTORY}/index.html" 
                    "<!DOCTYPE html>\n<html><head>\n  <meta http-equiv=\"refresh\" content=\"0;url=html/index.html\" />\n</head><body>\n  <p>If you are not redirected automatically, <a href=\"html/index.html\">click here</a>.</p>\n</body></html>\n")


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
