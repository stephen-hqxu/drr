#pragma once

#include <DisRegRep/Splatting/Convolution/Full/Base.hpp>
#include <DisRegRep/Splatting/Convolution/Base.hpp>

/**
 * @brief Precomputed region feature splatting dataset.
 */
namespace DisRegRep::Test::Splatting::GroundTruth {

using BaseFullConvolution = DisRegRep::Splatting::Convolution::Full::Base;
using BaseConvolution = DisRegRep::Splatting::Convolution::Base;

/**
 * @brief Check the minimum requirements of various splatting.
 *
 * @param splatting Splatting method.
 */
void checkMinimumRequirement(BaseConvolution&);

/**
 * @brief Check splatting coefficients computed by various splatting.
 *
 * @param splatting Splatting method.
 */
void checkSplattingCoefficient(BaseFullConvolution&);

}