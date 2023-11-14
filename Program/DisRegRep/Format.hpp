#pragma once

#include "Container/FixedHeapArray.hpp"

#include <array>
#include <cstdint>

namespace DisRegRep {

/**
 * @brief Standard type for filter format.
*/
namespace Format {

using RegionMap_t = std::uint8_t;/**< Pixel format of region map. */
using DenseBin_t = std::uint16_t;/**< Format of bin that stores region count. */
using DenseNormBin_t = float;/**< Format of bin that stores normalised weight. */

using DenseSingleHistogram = FixedHeapArray<DenseBin_t>;/**< Unnormalised bins */
using DenseNormSingleHistogram = FixedHeapArray<DenseNormBin_t>;/**< Normalised bins. */

/**
 * @brief A region map.
*/
struct RegionMap {

	FixedHeapArray<RegionMap_t> Map;/**< Region map data. */

	std::array<size_t, 2u> Dimension;/**< The dimension of region map. */
	size_t RegionCount;/**< The total number of contiguous region presented on region map. */

};

}

}