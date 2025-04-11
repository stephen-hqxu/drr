#pragma once

#include "../Base.hpp"

namespace DisRegRep::Splatting::OccupancyConvolution::Full {

/**
 * @brief As being a full convolution, all elements covered by the kernel are taken to derive the region occupancy.
 */
class Base : public OccupancyConvolution::Base { };

}