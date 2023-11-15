#pragma once

#include <DisRegRep/ProgramExport.hpp>
#include "../Format.hpp"

#include <array>

namespace DisRegRep {

/**
 * @brief A base class for implementation to create a region map.
*/
class DRR_API RegionMapFactory {
public:

	/**
	 * @brief Description about a region map.
	*/
	struct CreateDescription {

		size_t RegionCount;/**< The number of region to be used. */

	};

	constexpr RegionMapFactory() noexcept = default;

	RegionMapFactory(const RegionMapFactory&) = delete;

	RegionMapFactory(RegionMapFactory&&) = delete;

	constexpr virtual ~RegionMapFactory() = default;

	/**
	 * @brief Allocate a region map with given dimension.
	 * REMINDER: Implementation of this function resides in RandomRegionFactory.cpp
	 * 
	 * @param dimension The dimension of region map.
	 * 
	 * @return The allocated region map. The content of this map is undefined.
	*/
	Format::RegionMap allocateRegionMap(const Format::SizeVec2& dimension) const;

	/**
	 * @brief Create a region map.
	 * 
	 * @param desc The generation description.
	 * @param output Created region map.
	*/
	virtual void operator()(const RegionMapFactory::CreateDescription& desc, Format::RegionMap& output) const = 0;

	/**
	 * @brief This is a shortcut function. Allocate a new region map, and fill region map with content.
	 * 
	 * @param desc The generation description.
	 * @param dimension The dimension of region map.
	 * 
	 * @return The created region map.
	*/
	Format::RegionMap operator()(const RegionMapFactory::CreateDescription& desc, const Format::SizeVec2& dimension) const;

};

}