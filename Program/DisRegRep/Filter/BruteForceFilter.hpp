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

	constexpr std::string_view name() const noexcept override {
		return "BFF";
	}

	void tryAllocateHistogram(const LaunchDescription& desc, std::any& output) const override;

	const Format::DenseNormSingleHistogram& operator()(LaunchTag::Dense tag_dense,
		const LaunchDescription& desc, std::any& memory) const override;

};

}