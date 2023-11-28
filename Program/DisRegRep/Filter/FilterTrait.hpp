#pragma once

#include "RegionMapFilter.hpp"
#include "../Format.hpp"

#include <memory>

#include <any>

#include <concepts>

//No trailing semicolon is allowed
#define DEFINE_ALL_REGION_MAP_FILTER_ALLOC_FUNC(FILTER, SHORT_PREFIX) \
REGION_MAP_FILTER_ALLOC_FUNC_DCDH_DEF(FILTER) { DisRegRep::FilterTrait::makeAllocation<SHORT_PREFIX##dcdh>(desc, output); } \
REGION_MAP_FILTER_ALLOC_FUNC_DCSH_DEF(FILTER) { DisRegRep::FilterTrait::makeAllocation<SHORT_PREFIX##dcsh>(desc, output); } \
REGION_MAP_FILTER_ALLOC_FUNC_SCSH_DEF(FILTER) { DisRegRep::FilterTrait::makeAllocation<SHORT_PREFIX##scsh>(desc, output); }
#define DEFINE_ALL_REGION_MAP_FILTER_FILTER_FUNC_SCSH_DEF(FILTER, SHORT_PREFIX) \
REGION_MAP_FILTER_FILTER_FUNC_DCDH_DEF(FILTER) { return ::runFilter<SHORT_PREFIX##dcdh>(desc, output); } \
REGION_MAP_FILTER_FILTER_FUNC_DCSH_DEF(FILTER) { return ::runFilter<SHORT_PREFIX##dcsh>(desc, output); } \
REGION_MAP_FILTER_FILTER_FUNC_SCSH_DEF(FILTER) { return ::runFilter<SHORT_PREFIX##scsh>(desc, output); }

namespace DisRegRep {

/**
 * @brief Common traits for internal filter implementation.
*/
namespace FilterTrait {

/**
 * @brief Each filter implementation should have its internal data structure that satisfies this trait.
*/
template<typename T>
concept InternalFilterDataStructure = requires(T t, Format::SizeVec2 extent,
	Format::Region_t region_count, Format::Radius_t radius) {
	t.resize(extent, region_count, radius);
	t.clear();
};

/**
 * @brief Attempt to make an allocation to the output histogram.
 * This allocation function makes allocation on demand, and only allocate memory if necessary.
 * 
 * @tparam T The type of internal data structure.
 * 
 * @param desc The launch description.
 * @param output The memory of allocated histogram.
*/
template<InternalFilterDataStructure T>
void makeAllocation(const RegionMapFilter::LaunchDescription& desc, std::any& output) {
	using T_sp = std::shared_ptr<T>;

	T* allocation;
	//compatibility check
	if (const auto* const filter_memory = std::any_cast<T_sp>(&output);
		filter_memory) {
		allocation = &**filter_memory;
	} else {
		allocation = &*output.emplace<T_sp>(std::make_shared_for_overwrite<T>());
	}
	allocation->resize(desc.Extent, desc.Map->RegionCount, desc.Radius);
}

}

}