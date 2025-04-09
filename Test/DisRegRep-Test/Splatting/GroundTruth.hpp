#pragma once

#include <DisRegRep/Splatting/OccupancyConvolution/Full/Base.hpp>
#include <DisRegRep/Splatting/OccupancyConvolution/Base.hpp>

/**
 * @brief Precomputed region feature splatting dataset.
 */
namespace DisRegRep::Test::Splatting::GroundTruth {

using BaseFullOccupancyConvolution = DisRegRep::Splatting::OccupancyConvolution::Full::Base;
using BaseOccupancyConvolution = DisRegRep::Splatting::OccupancyConvolution::Base;

/**
 * @brief Check the minimum requirements of various splatting.
 *
 * @param splatting Splatting method.
 */
void checkMinimumRequirement(BaseOccupancyConvolution&);

/**
 * @brief Check splatting coefficients computed by various splatting.
 *
 * @param splatting Splatting method.
 */
void checkSplattingCoefficient(BaseFullOccupancyConvolution&);

}