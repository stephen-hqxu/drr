#pragma once

#include "RegionMapFilter.hpp"

namespace DisRegRep {

/**
 * @brief A unoptimised implementation to filter the region map and generate a region descriptor blend histogram, which is
 * expected to be very slow.
 */
class OriginalRegionAreaFilter final : public RegionMapFilter {
public:

	constexpr OriginalRegionAreaFilter() noexcept = default;

	constexpr ~OriginalRegionAreaFilter() override = default;

	constexpr std::string_view name() const noexcept override {
		return "ReAB";
	}

	REGION_MAP_FILTER_ALLOC_FUNC_ALL;
	REGION_MAP_FILTER_FILTER_FUNC_ALL;

};

}