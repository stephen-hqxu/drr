add_library(${DRR_CURRENT_TARGET} STATIC)
add_subdirectory(Container)
add_subdirectory(Core)
add_subdirectory(RegionfieldGenerator)
add_subdirectory(Splatting)

target_link_libraries(${DRR_CURRENT_TARGET}
PRIVATE
	${PROJECT_NAME}::xxHash
PUBLIC
	glm::glm
	range-v3::range-v3
)

drrSourceGroup()