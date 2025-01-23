find_package(CLI11 2 CONFIG REQUIRED)
find_package(nanobench CONFIG REQUIRED)
find_package(yaml-cpp 0.8 CONFIG REQUIRED)

add_executable(${DRR_CURRENT_TARGET})
add_subdirectory(Profiler)

target_link_libraries(${DRR_CURRENT_TARGET} PRIVATE
	${PROJECT_NAME}::Library

	CLI11::CLI11
	nanobench::nanobench
	yaml-cpp::yaml-cpp
)

set_target_properties(${DRR_CURRENT_TARGET} PROPERTIES
	OUTPUT_NAME drr
)

drrSourceGroup()