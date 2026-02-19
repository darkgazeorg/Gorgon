target_sources(Gorgon
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/Source"
    FILES
		../Geometry.h
		Bounds.h
		Margin.h
		Line.h
		Point.h
		PointList.h
		PointProperty.h
		Rectangle.h
		Size.h
		SizeProperty.h
		Point3D.h
		Transform3D.h
)

if(SCRIPTING)
    target_sources(Gorgon
        PRIVATE
            Scripting.cpp
    )
endif()
