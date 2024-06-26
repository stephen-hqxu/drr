#pragma once

#include "../Format.hpp"
#include "../Container/RegionMap.hpp"
#include "../Container/BlendHistogram.hpp"

#include <any>
#include <string_view>

#define DEFINE_TAG(NAME, HIST_TYPE) struct NAME { \
	using HistogramType = BlendHistogram::HIST_TYPE; \
	constexpr static std::string_view TagName = #NAME; \
}

//declare `tryAllocateHistogram`
#define REGION_MAP_FILTER_ALLOC_FUNC(TAG) void tryAllocateHistogram(LaunchTag::TAG, \
const LaunchDescription&, std::any&) const
#define REGION_MAP_FILTER_ALLOC_FUNC_DCDH REGION_MAP_FILTER_ALLOC_FUNC(DCDH)
#define REGION_MAP_FILTER_ALLOC_FUNC_DCSH REGION_MAP_FILTER_ALLOC_FUNC(DCSH)
#define REGION_MAP_FILTER_ALLOC_FUNC_SCSH REGION_MAP_FILTER_ALLOC_FUNC(SCSH)
#define REGION_MAP_FILTER_ALLOC_FUNC_ALL \
REGION_MAP_FILTER_ALLOC_FUNC_DCDH override; \
REGION_MAP_FILTER_ALLOC_FUNC_DCSH override; \
REGION_MAP_FILTER_ALLOC_FUNC_SCSH override

//declare `operator()`
#define REGION_MAP_FILTER_FILTER_FUNC(TAG) \
const DisRegRep::RegionMapFilter::LaunchTag::TAG::HistogramType& operator()(LaunchTag::TAG, \
const LaunchDescription&, std::any&) const
#define REGION_MAP_FILTER_FILTER_FUNC_DCDH REGION_MAP_FILTER_FILTER_FUNC(DCDH)
#define REGION_MAP_FILTER_FILTER_FUNC_DCSH REGION_MAP_FILTER_FILTER_FUNC(DCSH)
#define REGION_MAP_FILTER_FILTER_FUNC_SCSH REGION_MAP_FILTER_FILTER_FUNC(SCSH)
#define REGION_MAP_FILTER_FILTER_FUNC_ALL \
REGION_MAP_FILTER_FILTER_FUNC_DCDH override; \
REGION_MAP_FILTER_FILTER_FUNC_DCSH override; \
REGION_MAP_FILTER_FILTER_FUNC_SCSH override

//define `tryAllocateHistogram`
#define REGION_MAP_FILTER_ALLOC_FUNC_DEF(CLASS, TAG) void CLASS::tryAllocateHistogram(LaunchTag::TAG, \
const LaunchDescription& desc, any& output) const
#define REGION_MAP_FILTER_ALLOC_FUNC_DCDH_DEF(CLASS) REGION_MAP_FILTER_ALLOC_FUNC_DEF(CLASS, DCDH)
#define REGION_MAP_FILTER_ALLOC_FUNC_DCSH_DEF(CLASS) REGION_MAP_FILTER_ALLOC_FUNC_DEF(CLASS, DCSH)
#define REGION_MAP_FILTER_ALLOC_FUNC_SCSH_DEF(CLASS) REGION_MAP_FILTER_ALLOC_FUNC_DEF(CLASS, SCSH)

//define `operator()`
#define REGION_MAP_FILTER_FILTER_FUNC_DEF(CLASS, TAG) \
const DisRegRep::RegionMapFilter::LaunchTag::TAG::HistogramType& CLASS::operator()(LaunchTag::TAG, \
const LaunchDescription& desc, any& output) const
#define REGION_MAP_FILTER_FILTER_FUNC_DCDH_DEF(CLASS) REGION_MAP_FILTER_FILTER_FUNC_DEF(CLASS, DCDH)
#define REGION_MAP_FILTER_FILTER_FUNC_DCSH_DEF(CLASS) REGION_MAP_FILTER_FILTER_FUNC_DEF(CLASS, DCSH)
#define REGION_MAP_FILTER_FILTER_FUNC_SCSH_DEF(CLASS) REGION_MAP_FILTER_FILTER_FUNC_DEF(CLASS, SCSH)

namespace DisRegRep {

/**
 * @brief A fundamental class for different implementation of region map filters.
*/
class RegionMapFilter {
public:

	/**
	 * @brief Tag to specify filter type.
	*/
	struct LaunchTag {

		DEFINE_TAG(DCDH, DenseNorm);/**< Dense cache dense histogram */
		DEFINE_TAG(DCSH, SparseNormSorted);/**< Dense cache sparse histogram */
		DEFINE_TAG(SCSH, SparseNormUnsorted);/**< Sparse cache sparse histogram */

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
	virtual REGION_MAP_FILTER_ALLOC_FUNC_DCDH = 0;
	virtual REGION_MAP_FILTER_ALLOC_FUNC_DCSH = 0;
	virtual REGION_MAP_FILTER_ALLOC_FUNC_SCSH = 0;

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
	 * @return The generated normalised blend histogram for this region map.
	 * The memory is held by the `memory` input.
	 * It's safe to cast away constness if the application wishes to.
	 * However it is not recommended to destroys this memory
	 *	if application needs to reuse this histogram for further filter tasks.
	*/
	virtual REGION_MAP_FILTER_FILTER_FUNC_DCDH = 0;
	virtual REGION_MAP_FILTER_FILTER_FUNC_DCSH = 0;
	virtual REGION_MAP_FILTER_FILTER_FUNC_SCSH = 0;

};

}

#undef DEFINE_TAG