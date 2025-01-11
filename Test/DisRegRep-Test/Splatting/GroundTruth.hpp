#pragma once

#include <DisRegRep/Splatting/BaseFullConvolution.hpp>

/**
 * @brief Precomputed region feature splatting dataset.
 */
namespace DisRegRep::Test::Splatting::GroundTruth {

/**
 * @brief Check splatting coefficients computed by full convolution.
 *
 * @param full_conv A full convolution splatting.
 */
void checkFullConvolution(DisRegRep::Splatting::BaseFullConvolution&);

}