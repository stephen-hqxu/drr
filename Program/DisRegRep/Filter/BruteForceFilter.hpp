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

	void tryAllocateHistogram(LaunchTag::Dense tag_dense,
		const LaunchDescription& desc, std::any& output) const override;
	void tryAllocateHistogram(LaunchTag::Sparse tag_sparse,
		const LaunchDescription& desc, std::any& output) const override;

	const SingleHistogram::DenseNorm& operator()(LaunchTag::Dense tag_dense,
		const LaunchDescription& desc, std::any& memory) const override;
	const SingleHistogram::SparseNorm& operator()(LaunchTag::Sparse tag_sparse,
		const LaunchDescription& desc, std::any& memory) const override;

};

}