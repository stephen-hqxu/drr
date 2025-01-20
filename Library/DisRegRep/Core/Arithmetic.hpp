#pragma once

#include "Range.hpp"

#include <glm/vec2.hpp>

#include <functional>
#include <ranges>

#include <concepts>
#include <type_traits>
#include <utility>

/**
 * @brief Standard algebraic operations.
 */
namespace DisRegRep::Core::Arithmetic {

/**
 * @brief Normalise each value in a range.
 *
 * @library STL
 *
 * @tparam R Range type.
 * @tparam Factor Normalising value type.
 *
 * @param r Input range of values.
 * @param factor Normalising factor. Each value in the range is multiplied by a reciprocal of this.
 *
 * @return Normalised range.
 */
inline constexpr auto Normalise = Range::RangeAdaptorClosure([]<std::ranges::viewable_range R, std::floating_point Factor>
	requires std::ranges::input_range<R> && std::is_convertible_v<std::ranges::range_const_reference_t<R>, Factor>
	(R&& r, const Factor factor) static constexpr noexcept(std::is_nothrow_constructible_v<std::decay_t<R>, R>) -> auto {
		using std::views::repeat, std::views::zip_transform, std::multiplies;
		return zip_transform(multiplies {}, std::forward<R>(r), repeat(Factor { 1 } / factor));
	});

/**
 * @brief View a range as a 2D range.
 *
 * @library STL
 *
 * @tparam R Range type.
 *
 * @param r Input range to be viewed as 2D.
 * @param stride Stride of the second axis. The first axis is assumed to have a stride of one in `r`.
 *
 * @return 2D view of `r`.
 */
inline constexpr auto View2d =
	Range::RangeAdaptorClosure([]<std::ranges::viewable_range R, std::integral Stride = std::ranges::range_difference_t<R>>
		requires std::ranges::forward_range<R>
		(R&& r, const Stride stride) static constexpr noexcept(
			std::is_nothrow_constructible_v<std::decay_t<R>, R>) -> std::ranges::view auto {
			using std::views::chunk;
			return std::forward<R>(r) | chunk(stride);
		});

/**
 * @brief View a range as a transposed 2D range.
 *
 * @library STL
 *
 * @tparam R Range type.
 * @tparam Stride Stride type.
 *
 * @param r Input range to be viewed as transposed 2D.
 * @param stride Stride of the second axis. The first axis is assumed to have a stride of one in `r`.
 *
 * @return Transposed 2D view of `r`.
 */
inline constexpr auto ViewTransposed2d = Range::RangeAdaptorClosure(
	[]<std::ranges::viewable_range R, std::integral Stride = std::ranges::range_difference_t<R>>
	requires std::ranges::forward_range<R>
	(R&& r, const Stride stride) static constexpr noexcept(noexcept(std::forward<R>(r) | std::views::all)) -> std::ranges::view auto {
		using std::views::all, std::views::iota, std::views::transform, std::views::drop;
		return iota(Stride {}, stride)
			| transform([r = std::forward<R>(r) | all, stride](const auto offset) constexpr noexcept {
				return r
					| drop(offset)
					| std::views::stride(stride);
			});
	});

/**
 * @brief View a sub-range of a column-major 2D range.
 *
 * @library STL
 *
 * @tparam OuterR Outer range type.
 * @tparam InnerR Inner range type.
 *
 * @note `InnerR` is not required to be an input range; only `OuterR` requires so because of `std::views::transform`.
 *
 * @param outer_r Input 2D range to be sliced.
 * @param offset Coordinate of the first element in the slice.
 * @param extent Size of the slice.
 *
 * @return 2D sub-range of `outer_r`.
 */
inline constexpr auto SubRange2d = Range::RangeAdaptorClosure([]<
	std::ranges::viewable_range OuterR,
	std::ranges::viewable_range InnerR = std::ranges::range_reference_t<OuterR>,
	std::integral Size = std::common_type_t<std::ranges::range_difference_t<OuterR>, std::ranges::range_difference_t<InnerR>>
> requires std::ranges::input_range<OuterR> (
	OuterR&& outer_r,
	const glm::vec<2U, Size> offset,
	const glm::vec<2U, Size> extent
) static constexpr noexcept(
	std::is_nothrow_constructible_v<std::decay_t<OuterR>, OuterR>
) -> std::ranges::view auto {
	using std::views::drop, std::views::take, std::views::transform;
	return std::forward<OuterR>(outer_r)
		| drop(offset.y)
		| take(extent.y)
		| transform([offset_x = offset.x, extent_x = extent.x]<typename R>(R&& inner_r) constexpr noexcept(
			std::is_nothrow_constructible_v<std::decay_t<R>, R>
		) {
			return std::forward<R>(inner_r)
				| drop(offset_x)
				| take(extent_x);
		});
});

}