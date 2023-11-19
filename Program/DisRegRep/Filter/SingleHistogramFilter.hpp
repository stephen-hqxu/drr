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