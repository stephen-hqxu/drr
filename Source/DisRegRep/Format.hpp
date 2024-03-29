#pragma once

#include <array>

#include <type_traits>
#include <cstdint>

namespace DisRegRep {

/**
 * @brief Standard type for filter format.
*/
namespace Format {

using Size_t = std::size_t;/**< Represent a 1D size. */
using SSize_t = std::make_signed_t<Size_t>;/**< Represent a signed 1D size. */

template<size_t S>
using SizeVecN = std::array<Size_t, S>;/**< Represent an N-D size. */
using SizeVec2 = SizeVecN<2u>;/**< Represent a 2D size. */
using SizeVec3 = SizeVecN<3u>;/**< Represent a 3D size. */

using Radius_t = std::uint16_t;/**< Type of radius. */
using Region_t = std::uint8_t;/**< Type of region, which is also the pixel format of region map. */
using Bin_t = std::uint32_t;/**< Format of bin that stores region count. */
using NormBin_t = float;/**< Format of bin that stores normalised weight. */

}

}