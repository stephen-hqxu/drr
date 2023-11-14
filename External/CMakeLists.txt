add_library(${DRR_EXTERNAL} STATIC
	Implementation.cpp
)

target_sources(${DRR_EXTERNAL} PRIVATE
	nb/nanobench.h
)

target_include_directories(${DRR_EXTERNAL}
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)