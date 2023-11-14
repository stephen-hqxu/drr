#pragma once

#include "RegionMapFilter.hpp"

#include <any>

namespace DisRegRep {

/**
 * @brief A brute-force implementation of region map filter, which is expected to be very slow.
*/
class BruteForceFilter : public RegionMapFilter {
public:

	constexpr BruteForceFilter() noexcept = default;

	constexpr ~BruteForceFilter() override = default;

	//Only extent and region count of region map matters.
	std::any allocateHistogram(const RegionMapFilter::LaunchDescription& desc) const override;

	const Format::DenseNormSingleHistogram& filter(const RegionMapFilter::LaunchDescription& desc, std::any& memory) const override;

};

}