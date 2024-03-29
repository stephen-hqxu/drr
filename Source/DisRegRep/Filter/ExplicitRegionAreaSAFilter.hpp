#pragma once

#include "RegionMapFilter.hpp"

namespace DisRegRep {

/**
 * @brief Our proposed method for blazing-fast filtering the region map.
*/
class ExplicitRegionAreaSAFilter : public RegionMapFilter {
public:

	constexpr ExplicitRegionAreaSAFilter() noexcept = default;

	constexpr ~ExplicitRegionAreaSAFilter() override = default;

	constexpr std::string_view name() const noexcept override {
		return "ExRA+SA";
	}

	REGION_MAP_FILTER_ALLOC_FUNC_ALL;
	REGION_MAP_FILTER_FILTER_FUNC_ALL;

};

}