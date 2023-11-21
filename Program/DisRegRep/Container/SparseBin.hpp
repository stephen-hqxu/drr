#pragma once

#include "ContainerTrait.hpp"
#include "../Format.hpp"

#include <ranges>
#include <concepts>

namespace DisRegRep {

/**
 * @brief A data structure for holding histogram bin in a sparse data structure.
*/
namespace SparseBin {

/**
 * @brief The bin type for all sparse histogram implementations.
 * 
 * @tparam V The type of histogram bin value.
*/
template<typename V>
struct BasicSparseBin {

	using value_type = V;

	Format::Region_t Region;/**< The region this bin is storing. */
	value_type Value;/**< Bin value for this region. */

	constexpr bool operator==(const BasicSparseBin&) const noexcept = default;

};
using Bin = BasicSparseBin<Format::Bin_t>;
using NormBin = BasicSparseBin<Format::NormBin_t>;

//Check if a range is sparse bin.
template<typename R>
concept SparseBinRange = ContainerTrait::ValueRange<
	R,
	BasicSparseBin<typename std::ranges::range_value_t<R>::value_type>
>;

//Check if a range is sparse bin with a specific value type.
template<typename R, typename T>
concept SparseBinRangeValue = SparseBinRange<R>
	&& std::same_as<typename std::ranges::range_value_t<R>::value_type, T>;

}

}