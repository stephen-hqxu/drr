#pragma once

#include <DisRegRep/Range.hpp>
#include <DisRegRep/Type.hpp>

#include <range/v3/view/iota.hpp>

#include <tuple>

#include <algorithm>
#include <functional>
#include <ranges>

#include <type_traits>
#include <utility>

#include <cassert>

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

	[[nodiscard]] constexpr bool operator==(const Basic&) const
		noexcept(noexcept(std::declval<ValueType>() == std::declval<ValueType>())) = default;

};

using Importance = Basic<Type::RegionImportance>; /**< Sparse region importance matrix element. */
using Mask = Basic<Type::RegionMask>; /**< Sparse region mask matrix element. */

/**
 * `T` is a specialisation of basic sparse matrix element.
 */
template<typename T>
concept Is = std::is_same_v<T, Basic<typename T::ValueType>>;

/**
 * Type `R` models a range whose values are sparse importance elements.
 */
template<typename R>
concept ImportanceRange = std::is_same_v<std::ranges::range_value_t<R>, Importance>;

/**
 * @brief Format a range that contains region data in sparse matrix to use dense matrix for storage.
 *
 * @note The returned view is an input range.
 *
 * @library range-v3
 *
 * @tparam Sparse Type of sparse range.
 * @tparam Value Type of region data.
 *
 * @param sparse A range of sparse matrix element. The behaviour is undefined unless this range is sorted by region identifier.
 * @param region_count The total number of regions the dense matrix should contain.
 * @param fill_value A value used for filling in the dense matrix if it is not present in `sparse`.
 *
 * @return A range of dense matrix.
 */
inline constexpr auto ToDense = Range::RangeAdaptorClosure(
	[]<std::ranges::input_range Sparse, typename Value = typename std::ranges::range_value_t<Sparse>::ValueType>
	requires Is<std::ranges::range_value_t<Sparse>> && std::is_nothrow_copy_constructible_v<Value>
	(Sparse&& sparse, const Type::RegionIdentifier region_count, const Value fill_value = {}) static constexpr noexcept -> auto {
		using Range::ImpureTransform;
		using ranges::views::iota;
		using std::ranges::cbegin, std::ranges::cend,
			std::ranges::forward_range, std::ranges::is_sorted, std::mem_fn;
		if constexpr (forward_range<Sparse>) {
			//Must be sorted by region identifier
			//	because of how we forge a dense matrix by enumeration in ascending order.
			assert(is_sorted(sparse, {}, mem_fn(&Basic<Value>::Identifier)));
		}

		//We keep the current iterator as an internal state, making this range an input range (O(N)).
		//A way to maintain the original range category is
		//	by using binary search to find the element given a dense region ID (O(N log N)).
		return iota(Type::RegionIdentifier {}, region_count)
			 | ImpureTransform(
				 [fill_value, it = cbegin(sparse), end = cend(sparse)](const auto dense_region_id) mutable constexpr noexcept {
					 if (it == end) {
						 return fill_value;
					 }
					 if (const auto [sparse_region_id, value] = *it;
						 sparse_region_id == dense_region_id) [[unlikely]] {
						 ++it;
						 return value;
					 }
					 return fill_value;
				 });
	});

/**
 * @brief Format a range that contains dense region data to use sparse matrix for storage.
 *
 * @library STL
 *
 * @tparam Dense Type of dense range.
 * @tparam Value Type of region data.
 *
 * @param dense A range of region data.
 * @param ignore_value A value used for checking if a region data is considered ignored, and so ignored data are discarded from the
 * sparse range.
 *
 * @return A range of sparse matrix element.
 */
inline constexpr auto ToSparse =
	Range::RangeAdaptorClosure([]<std::ranges::viewable_range Dense, typename Value = std::ranges::range_value_t<Dense>>
		requires std::ranges::input_range<Dense> && std::equality_comparable<Value> && std::is_nothrow_copy_constructible_v<Value>
		(Dense&& dense, const Value ignore_value = {}) static constexpr noexcept -> auto {
			using std::make_from_tuple;
			using std::views::enumerate, std::views::filter, std::views::transform;
			return std::forward<Dense>(dense)
				 | enumerate
				 | filter([ignore_value](const auto it) constexpr noexcept { return std::get<1U>(it) != ignore_value; })
				 | transform([](const auto it) static constexpr noexcept { return make_from_tuple<Basic<Value>>(it); });
		});

}