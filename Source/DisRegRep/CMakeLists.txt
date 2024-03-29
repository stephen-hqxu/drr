add_executable(${DRR_MAIN}
	# Container/
	Container/HistogramCache.cpp
	Container/RegionMap.cpp
	Container/SingleHistogram.cpp
	# Factory/
	Factory/RandomRegionFactory.cpp
	Factory/RegionMapFactory.cpp
	Factory/VoronoiRegionFactory.cpp
	# Filter/
	Filter/BruteForceFilter.cpp
	Filter/SingleHistogramFilter.cpp
	# Launch/
	Launch/FilterRunner.cpp
	Launch/FilterTester.cpp
	Launch/Start.cpp
	Launch/ThreadPool.cpp
)

target_sources(${DRR_MAIN} PRIVATE
	# Container/
	Container/ContainerTrait.hpp
	Container/HistogramCache.hpp
	Container/RegionMap.hpp
	Container/SingleHistogram.hpp
	Container/SparseBin.hpp
	# Factory/
	Factory/RandomRegionFactory.hpp
	Factory/RegionMapFactory.hpp
	Factory/VoronoiRegionFactory.hpp
	# Filter/
	Filter/BruteForceFilter.hpp
	Filter/FilterTrait.hpp
	Filter/RegionMapFilter.hpp
	Filter/SingleHistogramFilter.hpp
	# Launch/
	Launch/FilterRunner.hpp
	Launch/FilterTester.hpp
	Launch/ThreadPool.hpp
	Launch/Utility.hpp
	# Maths/
	Maths/Arithmetic.hpp
	Maths/Indexer.hpp
	# /
	Format.hpp
)

cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH inc)
target_include_directories(${DRR_MAIN}
	PRIVATE ${inc}
)
target_link_libraries(${DRR_MAIN}
	PRIVATE ${DRR_EXTERNAL}
)

get_target_property(src ${DRR_MAIN} SOURCES)
source_group(
	TREE ${CMAKE_CURRENT_SOURCE_DIR}
	FILES ${src}
)

set_target_properties(${DRR_MAIN} PROPERTIES
	OUTPUT_NAME ${PROJECT_NAME}-Profiler
	VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${DRR_MAIN}>
)