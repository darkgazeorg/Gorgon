target_sources(Gorgon
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/Source"
    FILES
		../CGI.h
		Circle.h
		Line.h
		Path.h
		PathFill.h
		Polygon.h
)
