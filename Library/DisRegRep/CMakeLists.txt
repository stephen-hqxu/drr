add_library(${DRR_CURRENT_TARGET} STATIC)
add_subdirectory(Container)
add_subdirectory(Core)
add_subdirectory(RegionfieldGenerator)
add_subdirectory(Splatting)
drrTargetSource(
HEADER_TEMPLATE
	Info
)

target_link_libraries(${DRR_CURRENT_TARGET}
PRIVATE
	${PROJECT_NAME}::xxHash
PUBLIC
	glm::glm
)

drrSourceGroup()