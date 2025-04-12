#pragma once

#include <DisRegRep/Splatting/OccupancyConvolution/Full/Base.hpp>

/**
 * @brief Precomputed region feature splatting dataset.
 */
namespace DisRegRep::Test::Splatting::GroundTruth {

/**
 * @brief Check splatting coefficients computed by various splatting.
 *
 * @param splatting Splatting method.
 */
void checkSplattingCoefficient(DisRegRep::Splatting::OccupancyConvolution::Full::Base&);

}