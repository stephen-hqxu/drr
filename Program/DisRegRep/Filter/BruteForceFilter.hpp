#pragma once

#include "RegionMapFilter.hpp"

namespace DisRegRep {

/**
 * @brief A brute-force implementation of region map filter, which is expected to be very slow.
*/
class DRR_API BruteForceFilter : public RegionMapFilter {
public:

	constexpr BruteForceFilter() noexcept = default;

	constexpr ~BruteForceFilter() override = default;

	constexpr std::string_view name() const noexcept override {
		return "BFF";
	}

	REGION_MAP_FILTER_ALLOC_FUNC_ALL;
	REGION_MAP_FILTER_FILTER_FUNC_ALL;

};

}