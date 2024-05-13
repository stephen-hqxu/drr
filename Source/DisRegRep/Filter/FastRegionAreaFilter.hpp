#pragma once

#include "RegionMapFilter.hpp"

namespace DisRegRep {

/**
 * @brief Our proposed method for blazing-fast filtering the region map.
*/
class FastRegionAreaFilter final : public RegionMapFilter {
public:

	constexpr FastRegionAreaFilter() noexcept = default;

	constexpr ~FastRegionAreaFilter() override = default;

	constexpr std::string_view name() const noexcept override {
		return "ReAB+SA";
	}

	REGION_MAP_FILTER_ALLOC_FUNC_ALL;
	REGION_MAP_FILTER_FILTER_FUNC_ALL;

};

}