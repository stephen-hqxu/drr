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

	constexpr DescriptionContext::Type context() const noexcept override {
		using DC = DescriptionContext;
		return DC::Radius | DC::Extent | DC::RegionCount;
	}

	constexpr std::string_view name() const noexcept override {
		return "SHF";
	}

	std::any allocateHistogram(const LaunchDescription& desc) const override;

	const Format::DenseNormSingleHistogram& filter(const LaunchDescription& desc, std::any& memory) const override;

};

}