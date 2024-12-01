#pragma once

#include <glm/fwd.hpp>

#include <type_traits>

#include <cstdint>

/**
 * @brief Standard types defined to be used in the discrete region representation project.
 */
namespace DisRegRep::Type {

using Size = std::uint32_t; /**< Represent an unsigned 1D size. */
using SignedSize = std::make_signed_t<Size>; /**< Represent a signed 1D size. */

template<glm::length_t L>
using SizeVecN = glm::vec<L, Size>; /**< Represent an N-D size. */
using SizeVec2 = SizeVecN<2U>; /**< Represent a 2D size. */
using SizeVec3 = SizeVecN<3U>; /**< Represent a 3D size. */

using Radius = std::uint16_t;
using RegionIdentifier = std::uint8_t; /**< An integer to uniquely identify a region. */
using RegionImportance = std::uint32_t; /**< Region importance is defined as the frequency of occurence of a region. */
using RegionMask = glm::float32_t; /**< L1-normalised importance among all regions at the same coordinate. */

}