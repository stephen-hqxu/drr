#pragma once

#include "BaseFullConvolution.hpp"

namespace DisRegRep::Splatting {

/**
 * @brief An improved version of the vanilla convolution-based splatting coefficient computer by region occupancy, with two commonly
 * used convolution optimisations integrated, being separation and accumulation. Due to the use of separation and linear construction
 * of the splatting coefficient matrix, the output is transposed.
 *
 * @link VanillaFullOccupancy
 */
class FastFullOccupancy final : public BaseFullConvolution {
private:

	DRR_SPLATTING_DECLARE_DELEGATING_FUNCTOR_IMPL;

public:

	constexpr FastFullOccupancy() noexcept = default;

	constexpr ~FastFullOccupancy() override = default;

	DRR_SPLATTING_SET_INFO("F+", true)

	DRR_SPLATTING_DECLARE_FUNCTOR_ALL_IMPL;

};

}