#pragma once

#include <DisRegRep/Format.hpp>

#include <algorithm>

namespace DisRegRep::Launch {

/**
 * @brief Some handy tools...
*/
namespace Utility {

/**
 * @brief Calculate the minimum region map dimension.
 * 
 * @param extent The filter area extent.
 * @param radius The radius of kernel.
 * 
 * @return The minimum region map dimension.
*/
constexpr Format::SizeVec2 calcMinimumDimension(Format::SizeVec2 extent, const Format::Radius_t radius) {
	std::ranges::transform(extent, extent.begin(), [radius](const auto ext) constexpr noexcept { return ext + 2u * radius; });
	return extent;
}

/**
 * @brief Calculate the minimum region map offset for radius.
 * 
 * @param radius Specify radius of filter.
 * 
 * @return The minimum region map offset.
*/
constexpr Format::SizeVec2 calcMinimumOffset(const Format::Radius_t radius) {
	Format::SizeVec2 offset { };
	offset.fill(radius);
	return offset;
}

}

}