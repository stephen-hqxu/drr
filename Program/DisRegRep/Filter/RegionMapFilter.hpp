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

#define DEFINE_TAG(NAME) struct NAME { constexpr static std::string_view TagName = #NAME; }

//declare `tryAllocateHistogram`
#define REGION_MAP_FILTER_ALLOC_FUNC(TAG) void tryAllocateHistogram(LaunchTag::TAG, \
const LaunchDescription&, std::any&) const
#define REGION_MAP_FILTER_ALLOC_FUNC_DCDH REGION_MAP_FILTER_ALLOC_FUNC(DCacheDHist)
#define REGION_MAP_FILTER_ALLOC_FUNC_DCSH REGION_MAP_FILTER_ALLOC_FUNC(DCacheSHist)
#define REGION_MAP_FILTER_ALLOC_FUNC_SCSH REGION_MAP_FILTER_ALLOC_FUNC(SCacheSHist)
#define REGION_MAP_FILTER_ALLOC_FUNC_ALL \
REGION_MAP_FILTER_ALLOC_FUNC_DCDH override; \
REGION_MAP_FILTER_ALLOC_FUNC_DCSH override; \
REGION_MAP_FILTER_ALLOC_FUNC_SCSH override

//declare `operator()`
#define REGION_MAP_FILTER_FILTER_FUNC(RET, TAG) const SingleHistogram::RET& operator()(LaunchTag::TAG, \
const LaunchDescription&, std::any&) const
#define REGION_MAP_FILTER_FILTER_FUNC_DCDH REGION_MAP_FILTER_FILTER_FUNC(DenseNorm, DCacheDHist)
#define REGION_MAP_FILTER_FILTER_FUNC_DCSH REGION_MAP_FILTER_FILTER_FUNC(SparseNormSorted, DCacheSHist)
#define REGION_MAP_FILTER_FILTER_FUNC_SCSH REGION_MAP_FILTER_FILTER_FUNC(SparseNormUnsorted, SCacheSHist)
#define REGION_MAP_FILTER_FILTER_FUNC_ALL \
REGION_MAP_FILTER_FILTER_FUNC_DCDH override; \
REGION_MAP_FILTER_FILTER_FUNC_DCSH override; \
REGION_MAP_FILTER_FILTER_FUNC_SCSH override

//define `tryAllocateHistogram`
#define REGION_MAP_FILTER_ALLOC_FUNC_DEF(CLASS, TAG) void CLASS::tryAllocateHistogram(LaunchTag::TAG, \
const LaunchDescription& desc, any& output) const
#define REGION_MAP_FILTER_ALLOC_FUNC_DCDH_DEF(CLASS) REGION_MAP_FILTER_ALLOC_FUNC_DEF(CLASS, DCacheDHist)
#define REGION_MAP_FILTER_ALLOC_FUNC_DCSH_DEF(CLASS) REGION_MAP_FILTER_ALLOC_FUNC_DEF(CLASS, DCacheSHist)
#define REGION_MAP_FILTER_ALLOC_FUNC_SCSH_DEF(CLASS) REGION_MAP_FILTER_ALLOC_FUNC_DEF(CLASS, SCacheSHist)

//define `operator()`
#define REGION_MAP_FILTER_FILTER_FUNC_DEF(CLASS, RET, TAG) const SingleHistogram::RET& CLASS::operator()(LaunchTag::TAG, \
const LaunchDescription& desc, any& memory) const
#define REGION_MAP_FILTER_FILTER_FUNC_DCDH_DEF(CLASS) REGION_MAP_FILTER_FILTER_FUNC_DEF(CLASS, DenseNorm, DCacheDHist)
#define REGION_MAP_FILTER_FILTER_FUNC_DCSH_DEF(CLASS) REGION_MAP_FILTER_FILTER_FUNC_DEF(CLASS, SparseNormSorted, DCacheSHist)
#define REGION_MAP_FILTER_FILTER_FUNC_SCSH_DEF(CLASS) REGION_MAP_FILTER_FILTER_FUNC_DEF(CLASS, SparseNormUnsorted, SCacheSHist)

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

		DEFINE_TAG(DCacheDHist);/**< Dense cache dense histogram */
		DEFINE_TAG(DCacheSHist);/**< Dense cache sparse histogram */
		DEFINE_TAG(SCacheSHist);/**< Sparse cache sparse histogram */

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
	 * @return The generated normalised single histogram for this region map.
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