#pragma once

#include "../Format.hpp"

#include <array>

namespace DisRegRep {

/**
 * @brief A base class for implementation to create a region map.
*/
class RegionMapFactory {
public:

	/**
	 * @brief Description about a region map.
	*/
	struct CreateDescription {

		std::array<size_t, 2u> Dimension;/**< Dimension of region map. */
		size_t RegionCount;/**< The number of region to be used. */

	};

	constexpr RegionMapFactory() noexcept = default;

	constexpr virtual ~RegionMapFactory() = default;

	/**
	 * @brief Create a region map.
	 * 
	 * @param desc The generation description.
	 * 
	 * @return Created region map.
	*/
	virtual Format::RegionMap operator()(const RegionMapFactory::CreateDescription& desc) const = 0;

};

}