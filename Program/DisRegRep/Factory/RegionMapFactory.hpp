#pragma once

#include <DisRegRep/ProgramExport.hpp>
#include "../Format.hpp"

#include <array>
#include <string_view>

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
	 * 
	 * @param dimension The dimension of region map.
	 * 
	 * @return The allocated region map. The content of this map is undefined.
	*/
	[[nodiscard]] static Format::RegionMap allocate(const Format::SizeVec2& dimension);

	/**
	 * @brief Reshape the region map with a new dimension.
	 * A reallocation happens if new dimension is bigger than the region map.
	 * No allocation happens otherwise, but region map dimension will be set to the new dimension.
	 * 
	 * @param region_map The region map to be reshaped.
	 * The content of region map becomes undefined after this function returns if reallocation or enlargement happens.
	 * Otherwise, content will be trimmed.
	 * @param dimension The new dimension to be set.
	 * 
	 * @return True if reallocation happens, or region map has been enlarged.
	 * In both cases, the content of the original region map becomes undefined.
	*/
	static bool reshape(Format::RegionMap& region_map, const Format::SizeVec2& dimension);

	/**
	 * @brief Get an identifying name for the factory implementation.
	 * 
	 * @return The factory name.
	*/
	constexpr virtual std::string_view name() const noexcept = 0;

	/**
	 * @brief Create a region map.
	 * 
	 * @param desc The generation description.
	 * @param output Created region map.
	*/
	virtual void operator()(const CreateDescription& desc, Format::RegionMap& output) const = 0;

	/**
	 * @brief This is a shortcut function. Allocate a new region map, and fill region map with content.
	 * 
	 * @param desc The generation description.
	 * @param dimension The dimension of region map.
	 * 
	 * @return The created region map.
	*/
	[[nodiscard]] Format::RegionMap operator()(const CreateDescription& desc, const Format::SizeVec2& dimension) const;

};

}