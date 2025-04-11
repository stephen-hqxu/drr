#pragma once

#include "Base.hpp"

#include <DisRegRep/Container/Regionfield.hpp>

namespace DisRegRep::Splatting::OccupancyConvolution::Sampled {

/**
 * @brief Sample elements from the convolution kernel using a systematic sampling scheme, where elements are spaced in a regular
 * interval.
 */
class Systematic final : public Base {
public:

	DimensionType FirstSample = DimensionType(0U), /**< Coordinate of the first sample in the convolution kernel. */
		Interval = DimensionType(1U); /**< The number of elements to skip before taking the next sample. */

private:

	DRR_SPLATTING_DECLARE_DELEGATING_FUNCTOR_IMPL;

	void validate(const InvokeInfo&, const DisRegRep::Container::Regionfield&) const override;

public:

	DRR_SPLATTING_SET_INFO("S0", false)

	DRR_SPLATTING_DECLARE_SIZE_BYTE_IMPL;

	DRR_SPLATTING_DECLARE_FUNCTOR_ALL_IMPL;

};

}