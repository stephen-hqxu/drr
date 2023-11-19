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

	void tryAllocateHistogram(LaunchTag::Dense,
		const LaunchDescription&, std::any&) const override;
	void tryAllocateHistogram(LaunchTag::Sparse,
		const LaunchDescription&, std::any&) const override;

	const SingleHistogram::DenseNorm& operator()(LaunchTag::Dense,
		const LaunchDescription&, std::any&) const override;
	const SingleHistogram::SparseNorm& operator()(LaunchTag::Sparse,
		const LaunchDescription&, std::any&) const override;

};

}