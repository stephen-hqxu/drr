add_executable(${DRR_LAUNCH}
	Start.cpp
)

target_link_libraries(${DRR_LAUNCH}
	PRIVATE ${DRR_PROGRAM} ${DRR_EXTERNAL}
)