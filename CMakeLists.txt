cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(/W4 /arch:AVX2 /MP)
add_compile_options(/wd4251 /wd4275) # DLL export warning

project(DisRegRep
	LANGUAGES CXX
	DESCRIPTION "Discrete Region Representation: An implicit and explicit representation for discrete features and feature combination. Useful for fast procedural modelling of diverse landscape, and more."
	HOMEPAGE_URL https://github.com/stephen-hqxu/drr
)

set(CMAKE_STATIC_LIBRARY_PREFIX ${CMAKE_PROJECT_NAME}-)
set(CMAKE_SHARED_LIBRARY_PREFIX ${CMAKE_STATIC_LIBRARY_PREFIX})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/arc)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/bin)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

set(DRR_EXTERNAL "External")
set(DRR_LAUNCH "Launch")
set(DRR_PROGRAM "Program")

add_subdirectory(${DRR_EXTERNAL})
add_subdirectory(${DRR_PROGRAM}/${CMAKE_PROJECT_NAME})
add_subdirectory(${DRR_LAUNCH})