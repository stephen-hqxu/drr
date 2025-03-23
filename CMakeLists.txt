cmake_minimum_required(VERSION 3.23 FATAL_ERROR)

set(DRR_PROJECT_NAME "Discrete Region Representation")
project(DisRegRep
	VERSION 2
	LANGUAGES CXX
	DESCRIPTION "${DRR_PROJECT_NAME}: A mathematical model for discrete features and feature combination. Useful for fast procedural modelling of diverse landscape, and more."
	HOMEPAGE_URL https://github.com/stephen-hqxu/drr
)

##############
### Option ###
##############
option(DRR_ENABLE_TEST "Build test target" OFF)

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
if(CMAKE_CXX_COMPILER_ID STREQUAL Clang)
	add_compile_options(-Wno-missing-field-initializers)
endif()

##################
### Dependency ###
##################
find_package(glm 1 CONFIG REQUIRED)
find_package(Tiff 4 CONFIG REQUIRED)
find_package(xxHash 0.8 CONFIG REQUIRED)

add_subdirectory(External)

####################
### Target Setup ###
####################
#[[
Add source to target `DRR_CURRENT_TARGET`. Include directory for the source are set to `DRR_CURRENT_INCLUDE_DIR`, and for the generated are set to `DRR_CURRENT_GENERATED_INCLUDE_DIR`.

@param [HEADER <header>...] Target headers. All headers are assumed to be public.
@param [SOURCE <source>...] Target sources.
@param [HEADER_TEMPLATE <header template>...] Target header template. They will be configured and output to the designated generated directory.
]]
function(drrTargetSource)
	cmake_parse_arguments(TARGET "" "" "HEADER;SOURCE;HEADER_TEMPLATE" ${ARGV})

	list(TRANSFORM TARGET_HEADER APPEND .hpp)
	list(TRANSFORM TARGET_SOURCE APPEND .cpp)

	list(TRANSFORM TARGET_HEADER_TEMPLATE APPEND .hpp OUTPUT_VARIABLE GeneratedHeaderFilename)
	list(TRANSFORM TARGET_HEADER_TEMPLATE APPEND .hpp.in)
	foreach(input output IN ZIP_LISTS TARGET_HEADER_TEMPLATE GeneratedHeaderFilename)
		cmake_path(ABSOLUTE_PATH output)
		cmake_path(RELATIVE_PATH output BASE_DIRECTORY ${DRR_CURRENT_INCLUDE_DIR})
		cmake_path(ABSOLUTE_PATH output BASE_DIRECTORY ${DRR_CURRENT_GENERATED_INCLUDE_DIR})

		list(APPEND GeneratedHeader ${output})
		# Not using @ to identify replacement because we are using JavaDoc-style Doxygen that may confuse CMake.
		configure_file(${input} ${output} ESCAPE_QUOTES)
	endforeach()

	if(DRR_CURRENT_TARGET_REDISTRIBUTABLE)
		set(RedistributableVisibility PUBLIC)
	else()
		set(RedistributableVisibility PRIVATE)
	endif()

	target_sources(${DRR_CURRENT_TARGET}
	PRIVATE
		${TARGET_SOURCE}
	${RedistributableVisibility}
		FILE_SET HEADERS
			BASE_DIRS ${DRR_CURRENT_INCLUDE_DIR}
			FILES ${TARGET_HEADER}
		FILE_SET generated_header
			TYPE HEADERS
			BASE_DIRS ${DRR_CURRENT_GENERATED_INCLUDE_DIR}
			FILES ${GeneratedHeader}
	)
endfunction()

#[[
Setup source group for target `DRR_CURRENT_TARGET`.
]]
function(drrSourceGroup)
	get_target_property(Source ${DRR_CURRENT_TARGET} SOURCES)
	get_target_property(Header ${DRR_CURRENT_TARGET} HEADER_SET_HEADERS)
	get_target_property(GeneratedHeader ${DRR_CURRENT_TARGET} HEADER_SET_generated_header)

	foreach(Category IN ITEMS Source Header)
		source_group(
			TREE ${CMAKE_CURRENT_SOURCE_DIR}
			PREFIX ${Category}
			FILES ${${Category}}
		)
	endforeach()
	foreach(Category IN ITEMS GeneratedHeader)
		source_group(
			TREE ${DRR_CURRENT_GENERATED_DIR}
			PREFIX ${Category}
			FILES ${${Category}}
		)
	endforeach()
endfunction()

#[[
Add a subdirectory and set required environments.

@param <SourceDir> Directory of source to be added.
@param [REDISTRIBUTABLE] Set if the directory contains a target that shall be redistributed, i.e. will be linked by other targets, such as a library.
]]
function(drrAddSubdirectory SourceDir)
	cmake_parse_arguments(SUBDIR "REDISTRIBUTABLE" "" "" ${ARGN})

	set(DRR_CURRENT_TARGET ${SourceDir})
	set(DRR_CURRENT_TARGET_REDISTRIBUTABLE ${SUBDIR_REDISTRIBUTABLE})
	cmake_path(APPEND CMAKE_CURRENT_SOURCE_DIR ${SourceDir} OUTPUT_VARIABLE DRR_CURRENT_INCLUDE_DIR)

	set(Namespace ${PROJECT_NAME})
	if(NOT DRR_CURRENT_TARGET_REDISTRIBUTABLE)
		# Normally we do not need to worry about include directory for targets that will not be consumed by other targets.
		# Since CMake always adds target sources base directory to its include directory,
		#	we still need to setup a proper directory namespace.
		# For convenience, instead of having `<project name>/<target>/`,
		#	we use `<project name>-<target>/` to disambiguate from redistributable targets.
		string(APPEND Namespace -${DRR_CURRENT_TARGET})
	endif()

	cmake_path(APPEND PROJECT_BINARY_DIR Generated ${DRR_CURRENT_TARGET} OUTPUT_VARIABLE DRR_CURRENT_GENERATED_INCLUDE_DIR)
	cmake_path(APPEND DRR_CURRENT_GENERATED_INCLUDE_DIR ${Namespace} OUTPUT_VARIABLE DRR_CURRENT_GENERATED_DIR)
	# Always make a directory in case the target never has any generated file added, so the base directory points to a valid path (even if it is empty).
	file(MAKE_DIRECTORY ${DRR_CURRENT_GENERATED_INCLUDE_DIR})

	add_subdirectory(${DRR_CURRENT_TARGET}/${Namespace})

	if(DRR_CURRENT_TARGET_REDISTRIBUTABLE)
		add_library(${PROJECT_NAME}::${DRR_CURRENT_TARGET} ALIAS ${DRR_CURRENT_TARGET})
	else()
		# Assume non-redistributable targets are executable.
		set_target_properties(${DRR_CURRENT_TARGET} PROPERTIES
			VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${DRR_CURRENT_TARGET}>
		)
	endif()
endfunction()

###############################################################################
set(DRR_OUTPUT_ROOT ${PROJECT_BINARY_DIR}/Output)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${DRR_OUTPUT_ROOT}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${DRR_OUTPUT_ROOT}/lib)

set(CMAKE_STATIC_LIBRARY_PREFIX ${PROJECT_NAME}-)

drrAddSubdirectory(Library REDISTRIBUTABLE)
drrAddSubdirectory(Programme)
if(DRR_ENABLE_TEST)
	enable_testing()
	drrAddSubdirectory(Test)
endif()