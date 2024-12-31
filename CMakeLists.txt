cmake_minimum_required(VERSION 3.23 FATAL_ERROR)

project(DisRegRep
	LANGUAGES CXX
	DESCRIPTION "Discrete Region Representation: A mathematical model for discrete features and feature combination. Useful for fast procedural modelling of diverse landscape, and more."
	HOMEPAGE_URL https://github.com/stephen-hqxu/drr
)

################
### Language ###
################
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#####################
### Compiler Flag ###
#####################
if(MSVC)
	add_compile_options(/W4 /arch:AVX2 /MP)
else()
	add_compile_options(-Wall -mavx2 -mfma)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	add_compile_options(-Wno-missing-field-initializers)
endif()

##################
### Dependency ###
##################
find_package(CLI11 2 CONFIG REQUIRED)
find_package(glm 1 CONFIG REQUIRED)
find_package(nanobench CONFIG REQUIRED)
find_package(range-v3 0.12.0 CONFIG REQUIRED)
find_package(xxHash 0.8 CONFIG REQUIRED)
find_package(yaml-cpp 0.8 CONFIG REQUIRED)

add_subdirectory(External)

####################
### Target Setup ###
####################
#[[
Add source to target `DRR_CURRENT_TARGET`. Include directory of the source are set to `DRR_CURRENT_INCLUDE_DIR`.

@param [HEADER <header>...] Target headers.
@param [SOURCE <source>...] Target sources.
]]
function(drrTargetSource)
	cmake_parse_arguments(TARGET "" "" "HEADER;SOURCE" ${ARGV})

	list(TRANSFORM TARGET_HEADER APPEND .hpp)
	list(TRANSFORM TARGET_SOURCE APPEND .cpp)

	target_sources(${DRR_CURRENT_TARGET} PRIVATE
	FILE_SET header
		TYPE HEADERS
		BASE_DIRS ${DRR_CURRENT_INCLUDE_DIR}
		FILES ${TARGET_HEADER}
	)
	target_sources(${DRR_CURRENT_TARGET} PRIVATE ${TARGET_SOURCE})
endfunction()

#[[
Setup source group for target `DRR_CURRENT_TARGET`.
]]
function(drrSourceGroup)
	get_target_property(Source ${DRR_CURRENT_TARGET} SOURCES)
	get_target_property(Header ${DRR_CURRENT_TARGET} HEADER_SET_header)

	foreach(Category IN ITEMS Source Header)
		source_group(
			TREE ${CMAKE_CURRENT_SOURCE_DIR}
			PREFIX ${Category}
			FILES ${${Category}}
		)
	endforeach()
endfunction()

#[[
Add a subdirectory and set required environments.

@param <SourceDir> Directory of source to be added.
@param [INCLUDE_NAMESPACE] Specify if the include directory should be scoped under project namespace.
]]
function(drrAddSubdirectory SourceDir)
	cmake_parse_arguments(SUBDIR "INCLUDE_NAMESPACE" "" "" ${ARGN})

	set(DRR_CURRENT_TARGET ${SourceDir})
	cmake_path(APPEND CMAKE_CURRENT_SOURCE_DIR ${SourceDir} OUTPUT_VARIABLE DRR_CURRENT_INCLUDE_DIR)

	if(SUBDIR_INCLUDE_NAMESPACE)
		add_subdirectory(${SourceDir}/${PROJECT_NAME})
	else()
		add_subdirectory(${SourceDir})
	endif()
endfunction()

###############################################################################
set(DRR_OUTPUT_ROOT ${PROJECT_BINARY_DIR}/Output)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${DRR_OUTPUT_ROOT}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${DRR_OUTPUT_ROOT}/lib)

set(CMAKE_STATIC_LIBRARY_PREFIX ${PROJECT_NAME}-)

drrAddSubdirectory(Library INCLUDE_NAMESPACE)