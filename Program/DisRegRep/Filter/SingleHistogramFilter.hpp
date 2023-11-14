#pragma once

#include "RegionMapFilter.hpp"

#include <any>

namespace DisRegRep {

/**
 * @brief Meet SHF, our proposed method for lightening fast filtering region map.
*/
class SingleHistogramFilter : public RegionMapFilter {
public:

	constexpr SingleHistogramFilter() noexcept = default;

	constexpr ~SingleHistogramFilter() override = default;

	//Filter radius, extent and region count matters.
	std::any allocateHistogram(const RegionMapFilter::LaunchDescription& desc) const override;

	const Format::DenseNormSingleHistogram& filter(const RegionMapFilter::LaunchDescription& desc, std::any& memory) const override;

};

}