#pragma once

#include <array>
#include <cstdint>

namespace DisRegRep {

/**
 * @brief Standard type for filter format.
*/
namespace Format {

using SizeVec2 = std::array<size_t, 2u>;/**< An array of two size_t's. */
using Radius_t = std::uint16_t;/**< Type of radius. */

using Region_t = std::uint8_t;/**< Type of region, which is also the pixel format of region map. */
using Bin_t = std::uint32_t;/**< Format of bin that stores region count. */
using NormBin_t = float;/**< Format of bin that stores normalised weight. */

}

}