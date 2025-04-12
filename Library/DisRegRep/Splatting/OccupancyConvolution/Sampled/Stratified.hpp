#pragma once

#include "Base.hpp"

#include <DisRegRep/Container/Regionfield.hpp>

namespace DisRegRep::Splatting::OccupancyConvolution::Sampled {

/**
 * @brief Sample elements from the convolution kernel using a stratified sampling scheme. The kernel is divided into a number of
 * sub-kernels in equal-size, referred to strata, and one element is taken randomly from each stratum.
 */
class Stratified final : public Base {
public:

	KernelSizeType StratumCount = 1U; /**< Specify the number of strata arranged along the extent of the kernel. */
	SeedType Seed {}; /**< Seed the initial state of the stratified region sampler. */

private:

	DRR_SPLATTING_DECLARE_DELEGATING_FUNCTOR_IMPL;

	void validate(const InvokeInfo&, const DisRegRep::Container::Regionfield&) const override;

public:

	DRR_SPLATTING_SET_INFO("S1", false)

	DRR_SPLATTING_DECLARE_SIZE_BYTE_IMPL;

	DRR_SPLATTING_DECLARE_FUNCTOR_ALL_IMPL;

};

}