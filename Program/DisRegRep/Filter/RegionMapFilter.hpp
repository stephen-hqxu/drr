#pragma once

#include <DisRegRep/ProgramExport.hpp>
#include "../Format.hpp"
#include "../Container/RegionMap.hpp"
#include "../Container/SingleHistogram.hpp"

#include <any>
#include <string_view>

#include <algorithm>
#include <type_traits>
#include <concepts>

namespace DisRegRep {

/**
 * @brief A fundamental class for different implementation of region map filters.
*/
class DRR_API RegionMapFilter {
public:

	/**
	 * @brief Tag to specify filter type.
	*/
	struct LaunchTag {

		struct Dense { };/**< Dense matrix. */
		struct Sparse { };/**< Sparse matrix. */

	};

	/**
	 * @brief The configuration for filter launch.
	*/
	struct LaunchDescription {

		const RegionMap* Map;/**< The region map. */
		Format::SizeVec2 Offset,/**< Start offset. */
			Extent;/**< The extent of covered area to be filtered. */
		Format::Radius_t Radius;/**< Radius of filter. */

	};

	constexpr RegionMapFilter() noexcept = default;

	RegionMapFilter(const RegionMapFilter&) = delete;

	RegionMapFilter(RegionMapFilter&&) = delete;

	constexpr virtual ~RegionMapFilter() = default;

	/**
	 * @brief Get a descriptive name of the filter implementation.
	 * 
	 * @return The filter name.
	*/
	constexpr virtual std::string_view name() const noexcept = 0;

	/**
	 * @brief Try to allocate histogram memory for launch.
	 * This function first detects if the given `output` is sufficient to hold result for filtering using `desc`.
	 * If so, this function is a no-op; otherwise, memory will be allocated and placed to `output`.
	 * This can help minimising the number of reallocation.
	 * 
	 * @param tag_dense Specify to allocate dense histogram.
	 * @param tag_sparse Specify to allocate sparse histogram.
	 * @param desc The filter launch description.
	 * @param output The allocated memory.
	*/
	virtual void tryAllocateHistogram(LaunchTag::Dense tag_dense,
		const LaunchDescription& desc, std::any& output) const = 0;
	virtual void tryAllocateHistogram(LaunchTag::Sparse tag_sparse,
		const LaunchDescription& desc, std::any& output) const = 0;

	/**
	 * @brief Perform filter on region map.
	 * This filter does no boundary checking, application should adjust offset to handle padding.
	 * 
	 * @param tag_dense Specify to compute dense histogram.
	 * @param tag_sparse Specify to compute sparse histogram.
	 * @param desc The filter launch description.
	 * @param memory The memory allocated for this launch.
	 * The behaviour is undefined if this memory is not allocated with compatible launch description.
	 * The compatibility depends on implementation of derived class.
	 * 
	 * @return The generated normalised single histogram for this region map.
	 * The memory is held by the `memory` input.
	*/
	virtual const SingleHistogram::DenseNorm& operator()(LaunchTag::Dense tag_dense,
		const LaunchDescription& desc, std::any& memory) const = 0;
	virtual const SingleHistogram::SparseNorm& operator()(LaunchTag::Sparse tag_sparse,
		const LaunchDescription& desc, std::any& memory) const = 0;

};

}