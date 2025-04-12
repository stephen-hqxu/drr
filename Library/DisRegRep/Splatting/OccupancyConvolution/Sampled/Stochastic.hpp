#pragma once

#include "Base.hpp"

#include <DisRegRep/Container/Regionfield.hpp>

namespace DisRegRep::Splatting::OccupancyConvolution::Sampled {

/**
 * @brief Sample elements from the convolution kernel using a simple random sampling scheme. A fixed number of random samples are taken
 * among all elements in the kernel uniformly.
 */
class Stochastic final : public Base {
public:

	KernelSizeType Sample = 1U; /**< Number of samples to be taken from the convolution kernel. */
	SeedType Seed {}; /**< Seed the initial state of the stochastic region sampler. */

private:

	DRR_SPLATTING_DECLARE_DELEGATING_FUNCTOR_IMPL;

	void validate(const InvokeInfo&, const DisRegRep::Container::Regionfield&) const override;

public:

	DRR_SPLATTING_SET_INFO("S2", false)

	DRR_SPLATTING_DECLARE_SIZE_BYTE_IMPL;

	DRR_SPLATTING_DECLARE_FUNCTOR_ALL_IMPL;

};

}