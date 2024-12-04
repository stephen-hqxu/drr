#pragma once

#include <glm/fwd.hpp>

#include <cstdint>

/**
 * @brief Standard types defined to be used in the discrete region representation project.
 */
namespace DisRegRep::Type {

using Radius = std::uint16_t;
using RegionIdentifier = std::uint8_t; /**< An integer to uniquely identify a region. */
using RegionImportance = std::uint32_t; /**< Region importance is defined as the frequency of occurence of a region. */
using RegionMask = glm::float32_t; /**< L1-normalised importance among all regions at the same coordinate. */

}