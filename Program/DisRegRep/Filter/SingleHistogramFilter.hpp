#pragma once

#include "RegionMapFilter.hpp"

#include <any>

namespace DisRegRep {

/**
 * @brief Meet SHF, our proposed method for lightening fast filtering region map.
*/
class DRR_API SingleHistogramFilter : public RegionMapFilter {
public:

	constexpr SingleHistogramFilter() noexcept = default;

	constexpr ~SingleHistogramFilter() override = default;

	constexpr std::string_view name() const noexcept override {
		return "SHF";
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