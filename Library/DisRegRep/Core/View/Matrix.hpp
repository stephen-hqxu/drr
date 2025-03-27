#pragma once

#include "RangeAdaptorClosure.hpp"
#include "Trait.hpp"

#include <glm/vec2.hpp>

#include <ranges>

#include <utility>

#include <concepts>
#include <type_traits>

/**
 * @brief Manipulate a range as a multi-dimension matrix with support for basic linear algebraic operations.
 */
namespace DisRegRep::Core::View::Matrix {

/**
 * @brief Add an axis to the left of a range, such that the new left axis has a stride greater than that of the original axis.
 *
 * @tparam R Range type.
 *
 * @param r A new axis is added to the left of this range.
 * @param stride Stride of the new axis. The original axis is assumed to have a stride of one in `r`.
 *
 * @return `r` with a new axis added to the left.
 */
inline constexpr auto NewAxisLeft = RangeAdaptorClosure([]<typename R, typename Stride>
	requires std::is_invocable_v<decltype(std::views::chunk), R, Stride>
	(R&& r, const Stride stride) static constexpr noexcept(std::is_nothrow_invocable_v<decltype(std::views::chunk), R, Stride>)
		-> std::ranges::view auto {
			return std::forward<R>(r) | std::views::chunk(stride);
		});

/**
 * @brief Add an axis to the right of a range, such that the new right axis has a stride less than that of the original axis.
 *
 * @note This may bring performance degradation as it essentially creates a view to a transposed matrix, which will damage cache
 * locality.
 *
 * @tparam R Range type.
 * @tparam Stride Stride type.
 *
 * @param r A new axis is added to the right of this range.
 * @param stride Stride of the second axis. The first axis is assumed to have a stride of one in `r`.
 *
 * @return `r` with a new axis added to the right.
 */
inline constexpr auto NewAxisRight =
	RangeAdaptorClosure([]<std::ranges::viewable_range R, std::integral Stride>
		requires std::ranges::forward_range<R>
		(R&& r, const Stride stride) static constexpr noexcept(Trait::IsNothrowViewable<R>) -> std::ranges::view auto {
			using std::views::all, std::views::iota, std::views::transform, std::views::drop;
			return iota(Stride {}, stride) | transform([r = std::forward<R>(r) | all, stride](const auto offset) constexpr noexcept {
				return r | drop(offset) | std::views::stride(stride);
			});
		});

/**
 * @brief Create a 2D sub-range of a multidimensional range with at least two axes. It is assumed that the inner range has a stride of
 * one.
 *
 * @tparam OuterR Outer range type.
 * @tparam InnerR Inner range type.
 * @tparam OffsetSize, ExtentSize Size of offset and extent.
 *
 * @note `InnerR` is not required to be an input range; only `OuterR` requires so because of `std::views::transform`.
 *
 * @param outer_r Input range to be sliced.
 * @param offset Coordinate of the first element in the slice.
 * @param extent Size of the slice.
 *
 * @return 2D sub-range of `outer_r`.
 */
inline constexpr auto Slice2d = RangeAdaptorClosure([]<
	std::ranges::viewable_range OuterR,
	std::integral OffsetSize,
	std::integral ExtentSize,
	std::ranges::viewable_range InnerR = std::ranges::range_reference_t<OuterR>
> requires std::ranges::input_range<OuterR>
(OuterR&& outer_r, const glm::vec<2U, OffsetSize> offset, const glm::vec<2U, ExtentSize> extent) static constexpr noexcept(
	Trait::IsNothrowViewable<OuterR>
) -> std::ranges::view auto {
	using std::views::drop, std::views::take, std::views::transform;
	return std::forward<OuterR>(outer_r)
		| drop(offset.x)
		| take(extent.x)
		| transform(
			[offset_y = offset.y, extent_y = extent.y]<typename R>(R&& inner_r) constexpr noexcept(
				Trait::IsNothrowViewable<R>
			) {
				return std::forward<R>(inner_r)
					| drop(offset_y)
					| take(extent_y);
			});
});

}