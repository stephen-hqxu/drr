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
find_package(yaml-cpp 0.8 CONFIG REQUIRED)

###############################################################################
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/Output/bin)

set(DRR_MAIN "Main")

add_subdirectory(Source/${PROJECT_NAME})