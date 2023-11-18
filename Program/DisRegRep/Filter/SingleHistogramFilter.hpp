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

	void tryAllocateHistogram(const LaunchDescription& desc, std::any& output) const override;

	const Format::DenseNormSingleHistogram& operator()(LaunchTag::Dense tag_dense,
		const LaunchDescription& desc, std::any& memory) const override;

};

}