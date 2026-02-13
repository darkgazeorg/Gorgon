cmake_minimum_required(VERSION 3.25)

OPTION(BUILD_DOCUMENTATION "Use Doxygen to create the HTML based API documentation" ON)
OPTION(BUILD_PDF "Use Doxygen to create the PDF API documentation" OFF)
OPTION(SILENCE_DOXYGEN_WARNINGS "Silence Doxygen warnings" ON)

IF(BUILD_DOCUMENTATION)
	FIND_PACKAGE(Doxygen)
	IF(NOT DOXYGEN_FOUND)
		MESSAGE(FATAL_ERROR "Doxygen is required to build the documentation.")
	ENDIF()
	
	SET(DOXYGEN_HTML ${CMAKE_DOCUMENT_OUTPUT_DIRECTORY}/HTML/index.html)
	
	# control whether doxygen prints warnings; mapped into Doxyfile at configure time
	IF(SILENCE_DOXYGEN_WARNINGS)
		SET(DOXY_WARNINGS "NO")
	ELSE()
		SET(DOXY_WARNINGS "YES")
	ENDIF()
	
	SET(AllStr "")
	
	FOREACH(s ${All})
		IF(NOT ${s} MATCHES "(^Source/External)|(^Source/Gorgon/External)")
			SET(AllStr "${AllStr} \"${CMAKE_CURRENT_SOURCE_DIR}/${s}\"")
		ENDIF()
	ENDFOREACH()
	
	SET(DOCS_PATH ${CMAKE_SOURCE_DIR}/Docs)
	
	# Ensure path values in the configured Doxyfile are quoted so Doxygen does
	# not split paths containing spaces into multiple sources (which causes
	# warnings like "source '.../Additional' is not a readable file...").
	CONFIGURE_FILE(Scripts/Doxyfile.in ${PROJECT_BINARY_DIR}/Doxyfile @ONLY IMMEDIATE)
	
	ADD_CUSTOM_COMMAND(OUTPUT ${DOXYGEN_HTML}
		COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_BINARY_DIR}/Doxyfile
		SOURCES ${PROJECT_BINARY_DIR}/Doxyfile
		MAIN_DEPENDENCY Scripts/Doxyfile.in ${PROJECT_BINARY_DIR}/Doxyfile
		DEPENDS ${DOCS_PATH}/Design.html ${All}
	)
	
	ADD_CUSTOM_TARGET(docs ALL DEPENDS ${DOXYGEN_HTML})
	
	IF(BUILD_PDF)
		ADD_CUSTOM_TARGET(pdf ALL
			COMMAND make -C "${CMAKE_DOCUMENT_OUTPUT_DIRECTORY}/Latex" >pdflog
			DEPENDS Scripts/Doxyfile.in ${PROJECT_BINARY_DIR}/Doxyfile ${CMAKE_DOCUMENT_OUTPUT_DIRECTORY}/Latex
		)
		
		CONFIGURE_FILE(${CMAKE_DOCUMENT_OUTPUT_DIRECTORY}/Latex/refman.pdf ${CMAKE_DOCUMENT_OUTPUT_DIRECTORY}/Gorgon.pdf COPYONLY)
	ENDIF()
	
	INSTALL(DIRECTORY ${CMAKE_DOCUMENT_OUTPUT_DIRECTORY}/HTML DESTINATION share/doc/Gorgon)
ENDIF()
