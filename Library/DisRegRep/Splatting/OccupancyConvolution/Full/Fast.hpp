#pragma once

#include "Base.hpp"

namespace DisRegRep::Splatting::OccupancyConvolution::Full {

/**
 * @brief An improved version over the vanilla occupancy convolution, with two commonly used convolution optimisations integrated,
 * being separation and accumulation. Due to the use of separation and linear construction of the splatting coefficient matrix, the
 * output is transposed.
 */
class Fast final : public Base {
private:

	DRR_SPLATTING_DECLARE_DELEGATING_FUNCTOR_IMPL;

public:

	DRR_SPLATTING_SET_INFO("F+", true)

	DRR_SPLATTING_DECLARE_SIZE_BYTE_IMPL;

	DRR_SPLATTING_DECLARE_FUNCTOR_ALL_IMPL;

};

}