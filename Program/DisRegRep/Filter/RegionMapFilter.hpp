#pragma once

#include "../Format.hpp"

#include <array>
#include <any>

#include <algorithm>
#include <type_traits>
#include <concepts>

namespace DisRegRep {

/**
 * @brief A fundamental class for different implementation of region map filters.
*/
class RegionMapFilter {
public:

	/**
	 * @brief The configuration for filter launch.
	*/
	struct LaunchDescription {

		const Format::RegionMap* Map;/**< The region map. */
		std::array<size_t, 2u> Offset,/**< Start offset. */
			Extent;/**< The extent of covered area to be filtered. */
		size_t Radius;/**< Radius of filter. */

	};

public:

	constexpr RegionMapFilter() noexcept = default;

	constexpr virtual ~RegionMapFilter() = default;

	/**
	 * @brief Allocate histogram memory for launch.
	 * 
	 * @param desc The filter launch description.
	 * 
	 * @return Allocated memory.
	*/
	virtual std::any allocateHistogram(const RegionMapFilter::LaunchDescription& desc) const = 0;

	/**
	 * @brief Perform filter on region map.
	 * This filter does no boundary checking, application should adjust offset to handle padding.
	 * 
	 * @param desc The filter launch description.
	 * @param memory The memory allocated for this launch.
	 * The behaviour is undefined if this memory is not allocated with compatible launch description.
	 * The compatibility depends on implementation of derived class.
	 * 
	 * @return The generated normalised single histogram for this region map.
	 * The memory is held by the `memory` input.
	*/
	virtual const Format::DenseNormSingleHistogram& filter(const RegionMapFilter::LaunchDescription& desc,
		std::any& memory) const = 0;

};

}