#pragma once

#include "RegionMapFilter.hpp"

#include <any>

namespace DisRegRep {

/**
 * @brief A brute-force implementation of region map filter, which is expected to be very slow.
*/
class DRR_API BruteForceFilter : public RegionMapFilter {
public:

	constexpr BruteForceFilter() noexcept = default;

	constexpr ~BruteForceFilter() override = default;

	constexpr RegionMapFilter::DescriptionContext::Type context() const noexcept override {
		using DC = RegionMapFilter::DescriptionContext;
		return DC::Extent | DC::RegionCount;
	}

	constexpr std::string_view name() const noexcept override {
		return "BF";
	}

	std::any allocateHistogram(const RegionMapFilter::LaunchDescription& desc) const override;

	const Format::DenseNormSingleHistogram& filter(const RegionMapFilter::LaunchDescription& desc, std::any& memory) const override;

};

}