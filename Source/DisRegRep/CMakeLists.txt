cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH INCLUDE_DIR)

function(addMainComponent)
	cmake_parse_arguments(COMPONENT "" "" "HEADER;SOURCE" ${ARGV})

	list(TRANSFORM COMPONENT_HEADER APPEND .hpp)
	list(TRANSFORM COMPONENT_SOURCE APPEND .cpp)

	target_sources(${DRR_MAIN} PRIVATE
	FILE_SET header
		TYPE HEADERS
		BASE_DIRS ${INCLUDE_DIR}
		FILES ${COMPONENT_HEADER}
	)
	target_sources(${DRR_MAIN} PRIVATE ${COMPONENT_SOURCE})
endfunction()

#######################
### Target Creation ###
#######################
add_executable(${DRR_MAIN})

add_subdirectory(Container)
add_subdirectory(Profiler)
add_subdirectory(RegionfieldGenerator)
add_subdirectory(Splatting)
addMainComponent(
HEADER
	Arithmetic
	Exception
	Range
	Type
	UninitialisedAllocator
SOURCE
	Exception
	Main
)

target_link_libraries(${DRR_MAIN}
PRIVATE
	glm::glm
	nanobench::nanobench
	# Command Line API
	CLI11::CLI11
	yaml-cpp::yaml-cpp
	# Standard Library Extension
	range-v3::range-v3
)

set_target_properties(${DRR_MAIN} PROPERTIES
	RUNTIME_OUTPUT_NAME drr
	VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${DRR_MAIN}>
)

####################
### Source Group ###
####################
get_target_property(DRR_SOURCE ${DRR_MAIN} SOURCES)
get_target_property(DRR_HEADER ${DRR_MAIN} HEADER_SET_header)

function(addMainSourceGroup)
	cmake_parse_arguments(GROUP "" "PREFIX" "FILE" ${ARGV})
	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX ${GROUP_PREFIX} FILES ${GROUP_FILE})
endfunction()
addMainSourceGroup(PREFIX Source FILE ${DRR_SOURCE})
addMainSourceGroup(PREFIX Header FILE ${DRR_HEADER})