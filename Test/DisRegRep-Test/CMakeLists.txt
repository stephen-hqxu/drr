find_package(Catch2 3 CONFIG REQUIRED)
include(Catch)

add_executable(${DRR_CURRENT_TARGET})
add_subdirectory(Container)
add_subdirectory(Core)
# add_subdirectory(Splatting)

drrTargetSource(${DRR_CURRENT_TARGET}
HEADER
	StringMaker
)

target_link_libraries(${DRR_CURRENT_TARGET} PRIVATE
	${PROJECT_NAME}::Library

	Catch2::Catch2WithMain
	glm::glm
	range-v3::range-v3
)

string(TOLOWER ${DRR_CURRENT_TARGET} OutputName)
set_target_properties(${DRR_CURRENT_TARGET} PROPERTIES
	PREFIX drr-
	OUTPUT_NAME ${OutputName}
)

catch_discover_tests(${DRR_CURRENT_TARGET}
	WORKING_DIRECTORY $<TARGET_FILE_DIR:${DRR_CURRENT_TARGET}>
	REPORTER compact
)

drrSourceGroup()