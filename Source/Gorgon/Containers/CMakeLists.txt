target_sources(Gorgon
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/Source"
    FILES
		Collection.h
		GarbageCollection.h
		Hashmap.h
		Image.h
		Iterator.h
		Vector.h
		Wave.h
)
