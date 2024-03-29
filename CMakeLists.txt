cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(DisRegRep
	LANGUAGES CXX
	DESCRIPTION "Discrete Region Representation: An implicit and explicit representation for discrete features and feature combination. Useful for fast procedural modelling of diverse landscape, and more."
	HOMEPAGE_URL https://github.com/stephen-hqxu/drr
)

if(MSVC)
	add_compile_options(/W4 /arch:AVX2 /MP)
else()
	add_compile_options(-Wall -mavx2)
endif()

set(CMAKE_STATIC_LIBRARY_PREFIX ${PROJECT_NAME}-)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/output/arc)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/output/bin)

set(DRR_EXTERNAL "External")
set(DRR_MAIN "Main")

add_subdirectory(${DRR_EXTERNAL})
add_subdirectory(Source/${PROJECT_NAME})