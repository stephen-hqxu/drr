#pragma once

#include "RegionMapFilter.hpp"

namespace DisRegRep {

/**
 * @brief Meet SHF, our proposed method for lightening fast filtering region map.
*/
class SingleHistogramFilter : public RegionMapFilter {
public:

	constexpr SingleHistogramFilter() noexcept = default;

	constexpr ~SingleHistogramFilter() override = default;

	constexpr std::string_view name() const noexcept override {
		return "SHF";
	}

	REGION_MAP_FILTER_ALLOC_FUNC_ALL;
	REGION_MAP_FILTER_FILTER_FUNC_ALL;

};

}