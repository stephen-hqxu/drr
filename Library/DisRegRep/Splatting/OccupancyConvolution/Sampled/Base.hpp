#pragma once

#include "../Base.hpp"

namespace DisRegRep::Splatting::OccupancyConvolution::Sampled {

/**
 * @brief A sampled convolution only takes a subset of elements from the kernel to derive the region occupancy.
 */
class Base : public OccupancyConvolution::Base { };

}