#pragma once

#include "Base.hpp"

#include <DisRegRep/Container/Regionfield.hpp>

#include <random>

namespace DisRegRep::Splatting::OccupancyConvolution::Sampled {

/**
 * @brief Sample elements from the convolution kernel using a simple random sampling scheme. A fixed number of random samples are taken
 * among all elements in the kernel uniformly.
 */
class Stochastic final : public Base {
public:

	/**
	 * @brief Stochastic sampling requires scrambling all elements in the kernel, and repeat this process for every element on the
	 * regionfield. This mandates a random bit generator with a gargantuan period, where Mersenne-Twister becomes a perfect fit for
	 * this role.
	 *
	 * @note State space is associated with kernel size; Period is associated with regionfield and kernel size.
	 */
	using Sampler = std::mt19937;
	using SeedType = Sampler::result_type;

	KernelSizeType Sample = 1U; /**< Number of samples to be taken from the convolution kernel. */
	SeedType Seed = Sampler::default_seed; /**< Seed the initial state of the region sampler. */

private:

	DRR_SPLATTING_DECLARE_DELEGATING_FUNCTOR_IMPL;

	void validate(const InvokeInfo&, const DisRegRep::Container::Regionfield&) const override;

public:

	DRR_SPLATTING_SET_INFO("S2", false)

	DRR_SPLATTING_DECLARE_SIZE_BYTE_IMPL;

	DRR_SPLATTING_DECLARE_FUNCTOR_ALL_IMPL;

};

}