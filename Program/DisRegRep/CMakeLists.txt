include(GenerateExportHeader)

add_library(${DRR_PROGRAM} SHARED
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
	# /
	ThreadPool.cpp
)

target_sources(${DRR_PROGRAM} PRIVATE
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
	# Maths/
	Maths/Arithmetic.hpp
	Maths/Indexer.hpp
	# /
	Format.hpp
	ThreadPool.hpp
)

set(generated_include_dir ${CMAKE_CURRENT_BINARY_DIR}/GeneratedInclude)
generate_export_header(${DRR_PROGRAM}
	EXPORT_MACRO_NAME API
	EXPORT_FILE_NAME ${generated_include_dir}/${CMAKE_PROJECT_NAME}/${DRR_PROGRAM}Export.hpp
	INCLUDE_GUARD_NAME _DRR_PROGRAM_EXPORT_HPP_
	PREFIX_NAME DRR_
)

cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH inc)
target_include_directories(${DRR_PROGRAM}
	PUBLIC ${inc} ${generated_include_dir}
)

target_link_libraries(${DRR_PROGRAM}
	# we are only using it in source file
	PRIVATE ${DRR_EXTERNAL}
)

get_target_property(src ${DRR_PROGRAM} SOURCES)
source_group(
	TREE ${CMAKE_CURRENT_SOURCE_DIR}
	FILES ${src}
)