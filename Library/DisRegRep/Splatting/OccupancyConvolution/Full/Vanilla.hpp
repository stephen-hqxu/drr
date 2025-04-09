#pragma once

#include "Base.hpp"

namespace DisRegRep::Splatting::OccupancyConvolution::Full {

/**
 * @brief A vanilla (a.k.a. naive or brute-force) convolution to compute the region occupancy.
 */
class Vanilla final : public Base {
private:

	DRR_SPLATTING_DECLARE_DELEGATING_FUNCTOR_IMPL;

public:

	DRR_SPLATTING_SET_INFO("F-", false)

	DRR_SPLATTING_DECLARE_SIZE_BYTE_IMPL;

	DRR_SPLATTING_DECLARE_FUNCTOR_ALL_IMPL;

};

}