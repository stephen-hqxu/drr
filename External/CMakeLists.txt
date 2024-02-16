set(DRR_EXTERNAL_LIB_DST ${CMAKE_CURRENT_BINARY_DIR}/Download)

set(NANOBENCH_H ${DRR_EXTERNAL_LIB_DST}/nb/nanobench.h)
if(NOT EXISTS ${NANOBENCH_H})
	file(DOWNLOAD https://raw.githubusercontent.com/martinus/nanobench/v4.3.11/src/include/nanobench.h ${NANOBENCH_H}
		TIMEOUT 5
	)
endif()

add_library(${DRR_EXTERNAL} STATIC
	Implementation.cpp
)
target_sources(${DRR_EXTERNAL} PRIVATE
	${NANOBENCH_H}
)
target_include_directories(${DRR_EXTERNAL}
	PUBLIC ${DRR_EXTERNAL_LIB_DST}
)