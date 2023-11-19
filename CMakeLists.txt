cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(/W4 /arch:AVX2 /MP)
add_compile_options(/wd4251 /wd4275) # DLL export warning

project(DisRegRep
	VERSION 1.0.0
	DESCRIPTION "Benchmark for discrete region representation from my paper"
	LANGUAGES CXX
)

set(CMAKE_STATIC_LIBRARY_PREFIX ${CMAKE_PROJECT_NAME}-)
set(CMAKE_SHARED_LIBRARY_PREFIX lib${CMAKE_STATIC_LIBRARY_PREFIX})
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