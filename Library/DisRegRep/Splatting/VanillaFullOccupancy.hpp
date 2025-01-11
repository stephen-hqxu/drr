#pragma once

#include "BaseFullConvolution.hpp"

namespace DisRegRep::Splatting {

/**
 * @brief A vanilla (a.k.a. naive or brute-force) convolution-based splatting coefficient computer by the occupancy of each region.
 */
class VanillaFullOccupancy final : public BaseFullConvolution {
private:

	DRR_SPLATTING_DECLARE_DELEGATING_FUNCTOR_IMPL;

public:

	constexpr VanillaFullOccupancy() noexcept = default;

	constexpr ~VanillaFullOccupancy() override = default;

	DRR_SPLATTING_SET_INFO("F-", false)

	DRR_SPLATTING_DECLARE_FUNCTOR_ALL_IMPL;

};

}