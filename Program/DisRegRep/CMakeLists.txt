add_library(${DRR_PROGRAM} STATIC
	# Factory/
	Factory/RandomRegionFactory.cpp
	# Filter/
	Filter/BruteForceFilter.cpp
	Filter/SingleHistogramFilter.cpp
)

target_sources(${DRR_PROGRAM} PRIVATE
	# Container/
	Container/FixedHeapArray.hpp
	# Factory/
	Factory/RandomRegionFactory.hpp
	Factory/RegionMapFactory.hpp
	# Filter/
	Filter/BruteForceFilter.hpp
	Filter/RegionMapFilter.hpp
	Filter/SingleHistogramFilter.hpp
	# Maths/
	Maths/Arithmetic.hpp
	Maths/Indexer.hpp
	# /
	Format.hpp
)

cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH inc)
target_include_directories(${DRR_PROGRAM}
	PUBLIC ${inc}
)

get_target_property(src ${DRR_PROGRAM} SOURCES)
source_group(
	TREE ${CMAKE_CURRENT_SOURCE_DIR}
	FILES ${src}
)