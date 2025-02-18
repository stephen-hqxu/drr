#pragma once

#include "RangeAdaptorClosure.hpp"
#include "Trait.hpp"

#include <glm/vec2.hpp>

#include <ranges>

#include <utility>

#include <concepts>
#include <type_traits>

/**
 * @brief View a linear range as a multi-dimension matrix with support for basic linear algebraic operations.
 */
namespace DisRegRep::Core::View::Matrix {

/**
 * @brief View a range as a 2D range.
 *
 * @tparam R Range type.
 *
 * @param r Input range to be viewed as 2D.
 * @param stride Stride of the second axis. The first axis is assumed to have a stride of one in `r`.
 *
 * @return 2D view of `r`.
 */
inline constexpr auto View2d =
	RangeAdaptorClosure([]<std::ranges::viewable_range R, std::integral Stride = std::ranges::range_difference_t<R>>
		requires std::ranges::forward_range<R>
		(R&& r, const Stride stride) static constexpr noexcept(Trait::IsNothrowViewable<R>) -> std::ranges::view auto {
			using std::views::chunk;
			return std::forward<R>(r) | chunk(stride);
		});

/**
 * @brief View a range as a transposed 2D range.
 *
 * @tparam R Range type.
 * @tparam Stride Stride type.
 *
 * @param r Input range to be viewed as transposed 2D.
 * @param stride Stride of the second axis. The first axis is assumed to have a stride of one in `r`.
 *
 * @return Transposed 2D view of `r`.
 */
inline constexpr auto ViewTransposed2d =
	RangeAdaptorClosure([]<std::ranges::viewable_range R, std::integral Stride = std::ranges::range_difference_t<R>>
		requires std::ranges::forward_range<R>
		(R&& r, const Stride stride) static constexpr noexcept(Trait::IsNothrowViewable<R>) -> std::ranges::view auto {
			using std::views::all, std::views::iota, std::views::transform, std::views::drop;
			return iota(Stride {}, stride) | transform([r = std::forward<R>(r) | all, stride](const auto offset) constexpr noexcept {
				return r | drop(offset) | std::views::stride(stride);
			});
		});

/**
 * @brief View a sub-range of a 2D range. It is assumed that the inner range represents the Y axis that has a stride of one.
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
inline constexpr auto SubRange2d = RangeAdaptorClosure([]<
	std::ranges::viewable_range OuterR,
	std::ranges::viewable_range InnerR = std::ranges::range_reference_t<OuterR>,
	std::integral Size = std::common_type_t<std::ranges::range_difference_t<OuterR>,
	std::ranges::range_difference_t<InnerR>>
> requires std::ranges::input_range<OuterR>
	(OuterR&& outer_r, const glm::vec<2U, Size> offset, const glm::vec<2U, Size> extent) static constexpr noexcept(
		Trait::IsNothrowViewable<OuterR>) -> std::ranges::view auto {
		using std::views::drop, std::views::take, std::views::transform;
		return std::forward<OuterR>(outer_r)
			| drop(offset.x)
			| take(extent.x)
			| transform([offset_y = offset.y, extent_y = extent.y]<typename R>(R&& inner_r) constexpr noexcept(
				Trait::IsNothrowViewable<R>) {
				return std::forward<R>(inner_r)
					| drop(offset_y)
					| take(extent_y);
				});
	});

}