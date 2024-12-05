#pragma once

#include <glm/fwd.hpp>

#include <ranges>
#include <type_traits>

#include <cstdint>

/**
 * @brief Standard types defined to be used in the discrete region representation project.
 */
namespace DisRegRep::Type {

using Radius = std::uint16_t;
using RegionIdentifier = std::uint8_t; /**< An integer to uniquely identify a region. */
using RegionImportance = std::uint32_t; /**< Region importance is defined as the frequency of occurence of a region. */
using RegionMask = glm::float32_t; /**< L1-normalised importance among all regions at the same coordinate. */

/**
 * @brief Concept supports for predefined types.
 */
namespace Concept {

/**
 * Type `R` models an input range and the range value is convertible to region importance.
 */
template<typename R>
concept RegionImportanceInputRange =
	std::ranges::input_range<R> && std::is_convertible_v<std::ranges::range_value_t<R>, RegionImportance>;

}

}