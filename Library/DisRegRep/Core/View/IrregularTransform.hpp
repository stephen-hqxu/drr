#pragma once

#include "CacheLatest.hpp"
#include "RangeAdaptorClosure.hpp"
#include "Trait.hpp"

#include <iterator>
#include <ranges>

#include <utility>

#include <type_traits>

namespace DisRegRep::Core::View {

/**
 * @brief Apply a transformation to each element in the range. Differ from `std::views::transform`, the transformation function is
 * allowed to be irregular. The resulting range is single-pass due to caching.
 *
 * @tparam R Input range type.
 * @tparam F Transformation function type.
 *
 * @param r Input range whose elements are to be transformed.
 * @param f Transformation function to be applied.
 *
 * @return Transformed range.
 */
inline constexpr auto IrregularTransform = RangeAdaptorClosure([]<std::ranges::viewable_range R, RangeApplicator F>
	requires std::ranges::input_range<R> && std::indirectly_unary_invocable<F, std::ranges::iterator_t<R>>
	(R&& r, F f) static constexpr noexcept(
		Trait::IsNothrowViewable<R> && std::is_nothrow_move_constructible_v<F>) -> std::ranges::view auto {
		//This cache views force the range to be an input range, and each element in the preceding range will be dereferenced exactly
		//	once, thus eliminating the possibility of calling the irregular function more than once per iteration.
		return std::forward<R>(r) | std::ranges::views::transform(std::move(f)) | CacheLatest;
	});

}