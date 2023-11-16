#pragma once

#include <DisRegRep/ProgramExport.hpp>
#include "../Format.hpp"

#include <any>
#include <string_view>

#include <algorithm>
#include <type_traits>
#include <concepts>

#include <cstdint>

namespace DisRegRep {

/**
 * @brief A fundamental class for different implementation of region map filters.
*/
class DRR_API RegionMapFilter {
public:

	/**
	 * @brief Determines the related context a filter launch description.
	*/
	struct DescriptionContext {

		using Type = std::uint8_t;

		constexpr static DescriptionContext::Type RegionCount = 1u << 0u,
			Extent = 1u << 2u,
			Radius = 1u << 3u;

	};

	/**
	 * @brief The configuration for filter launch.
	*/
	struct LaunchDescription {

		const Format::RegionMap* Map;/**< The region map. */
		Format::SizeVec2 Offset,/**< Start offset. */
			Extent;/**< The extent of covered area to be filtered. */
		Format::Radius_t Radius;/**< Radius of filter. */

	};

	constexpr RegionMapFilter() noexcept = default;

	RegionMapFilter(const RegionMapFilter&) = delete;

	RegionMapFilter(RegionMapFilter&&) = delete;

	constexpr virtual ~RegionMapFilter() = default;

	/**
	 * @brief Determines the relevant context of the implemented filter.
	 * This is useful to determine the compatible histogram allocation with a particular description,
	 * and so if histogram should be reallocated.
	 * Any bits appear in the context are bounded to allocated histogram,
	 * so description settings of those fields must not be changed if histogram is to be reused.
	 * 
	 * @return The description context.
	*/
	constexpr virtual DescriptionContext::Type context() const noexcept = 0;

	/**
	 * @brief Get a descriptive name of the filter implementation.
	 * 
	 * @return The filter name.
	*/
	constexpr virtual std::string_view name() const noexcept = 0;

	/**
	 * @brief Allocate histogram memory for launch.
	 * 
	 * @param desc The filter launch description.
	 * 
	 * @return Allocated memory.
	*/
	virtual std::any allocateHistogram(const LaunchDescription& desc) const = 0;

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
	virtual const Format::DenseNormSingleHistogram& filter(const LaunchDescription& desc,
		std::any& memory) const = 0;

};

}