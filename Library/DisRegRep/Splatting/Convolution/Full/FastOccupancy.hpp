#pragma once

#include "Base.hpp"

namespace DisRegRep::Splatting::Convolution::Full {

/**
 * @brief An improved version of the vanilla convolution-based splatting coefficient computer by region occupancy, with two commonly
 * used convolution optimisations integrated, being separation and accumulation. Due to the use of separation and linear construction
 * of the splatting coefficient matrix, the output is transposed.
 *
 * @link VanillaOccupancy
 */
class FastOccupancy final : public Base {
private:

	DRR_SPLATTING_DECLARE_DELEGATING_FUNCTOR_IMPL;

public:

	constexpr FastOccupancy() noexcept = default;

	constexpr ~FastOccupancy() override = default;

	DRR_SPLATTING_SET_INFO("F+", true)

	DRR_SPLATTING_DECLARE_SIZE_BYTE_IMPL;

	DRR_SPLATTING_DECLARE_FUNCTOR_ALL_IMPL;

};

}