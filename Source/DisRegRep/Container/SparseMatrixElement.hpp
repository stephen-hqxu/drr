#pragma once

#include <DisRegRep/Type.hpp>

#include <ranges>
#include <type_traits>

/**
 * @brief Represent an element in a sparse matrix for storing data related to a region.
 */
namespace DisRegRep::Container::SparseMatrixElement {

/**
 * @brief Generic data structure of one element in a sparse matrix for storing region data.
 * 
 * @tparam V Sparse matrix element value type.
 */
template<typename V>
struct Basic {

	using ValueType = V;

	Type::RegionIdentifier Identifier; /**< Identifier of the region this element is for. */
	ValueType Value; /**< Data stored in this element for this region. */

	[[nodiscard]] constexpr bool operator==(const Basic&) const = default;

};

using Importance = Basic<Type::RegionImportance>; /**< Sparse region importance matrix element. */
using Mask = Basic<Type::RegionMask>; /**< Sparse region mask matrix element. */

/**
 * @brief Concept supports for sparse matrix element.
 */
namespace Concept {

/**
 * Type `R` models an input range of sparse matrix element.
 */
template<typename R>
concept ImportanceInputRange =
	std::ranges::input_range<R> && std::is_same_v<std::ranges::range_value_t<R>, Importance>;

}

}