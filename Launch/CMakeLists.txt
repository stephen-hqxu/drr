add_executable(${DRR_LAUNCH}
	FilterRunner.cpp
	FilterTester.cpp
	Start.cpp
)

target_sources(${DRR_LAUNCH} PRIVATE
	FilterRunner.hpp
	FilterTester.hpp
	Utility.hpp
)

target_link_libraries(${DRR_LAUNCH}
	PRIVATE ${DRR_PROGRAM} ${DRR_EXTERNAL}
)

set_target_properties(${DRR_LAUNCH} PROPERTIES
	VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${DRR_LAUNCH}>
)