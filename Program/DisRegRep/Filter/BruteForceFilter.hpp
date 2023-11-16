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

	constexpr DescriptionContext::Type context() const noexcept override {
		using DC = DescriptionContext;
		return DC::Extent | DC::RegionCount;
	}

	constexpr std::string_view name() const noexcept override {
		return "BF";
	}

	std::any allocateHistogram(const LaunchDescription& desc) const override;

	const Format::DenseNormSingleHistogram& filter(const LaunchDescription& desc, std::any& memory) const override;

};

}