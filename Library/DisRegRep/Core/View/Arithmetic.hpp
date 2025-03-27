#pragma once

#include "Functional.hpp"
#include "RangeAdaptorClosure.hpp"
#include "Trait.hpp"

#include <DisRegRep/Core/View/Concat.hpp>

#include <glm/fwd.hpp>

#include <functional>
#include <ranges>

#include <utility>

#include <concepts>
#include <type_traits>

/**
 * @brief Standard algebraic operations.
 */
namespace DisRegRep::Core::View::Arithmetic {

/**
 * `R` can be padded with clamp to edge addressing mode with size of type `Size`.
 */
template<typename R, typename Size>
concept ClampToEdgePaddableRange = std::ranges::bidirectional_range<R>
	&& std::ranges::common_range<R>
	&& std::ranges::sized_range<R>
	&& std::is_invocable_v<decltype(std::views::repeat), std::ranges::range_reference_t<R>, Size>;

/**
 * @brief Normalise each value in a range.
 *
 * @tparam R Range type.
 * @tparam Factor Normalising value type.
 *
 * @param r Input range of values.
 * @param factor Normalising factor. Each value in the range is multiplied by a reciprocal of this.
 *
 * @return Normalised range.
 */
inline constexpr auto Normalise = RangeAdaptorClosure([]<std::ranges::viewable_range R, std::floating_point Factor>
	requires std::ranges::input_range<R> && std::is_convertible_v<std::ranges::range_reference_t<R>, Factor>
	(R&& r, const Factor factor) static constexpr noexcept(Trait::IsNothrowViewable<R>) -> std::ranges::view auto {
		using std::views::transform, std::bind_front, std::multiplies;
		return std::forward<R>(r) | transform(bind_front(multiplies {}, Factor { 1 } / factor));
	});

/**
 * @brief Get a range of evenly spaced numbers over a specified interval in [from, to].
 *
 * @tparam T Interval type. This is also used as the range type.
 * @tparam N Step size type.
 *
 * @param from Interval start.
 * @param to Interval end.
 * @param n Number of step.
 *
 * @return Linearly spaced range.
 */
inline constexpr auto LinSpace = []<typename T, std::unsigned_integral N>
requires std::is_arithmetic_v<T>
(const T from, const std::type_identity_t<T> to, const N n) static constexpr noexcept -> std::ranges::view auto {
	using std::bind_front, std::plus, std::multiplies,
		std::views::iota, std::views::transform,
		std::cmp_greater,
		std::conditional_t, std::is_floating_point_v;
	using DeltaType = conditional_t<is_floating_point_v<T>, T, glm::float64_t>;
	return iota(N {}, n)
		| transform(bind_front(multiplies {}, cmp_greater(n, 1) ? (to - from) / static_cast<DeltaType>(n - 1) : 0))
		| transform(bind_front(plus {}, from))
		| Functional::Cast<T>;
};

/**
 * @brief Pad a range such that its size is no less than the specified size. For element outside the original range, the last element
 * is repeated to pad the size up to the specified.
 *
 * @tparam R Range type.
 * @tparam Size Padding size type.
 *
 * @param r Range to be padded.
 * @param total_size Total size of the range. No padding is performed if total size is no greater than the original range size.
 *
 * @return Range padded by its last element.
 */
inline constexpr auto PadClampToEdge = RangeAdaptorClosure([]<
	std::ranges::viewable_range R,
	typename Size
> requires ClampToEdgePaddableRange<R, Size>
(R&& r, const Size total_size) static constexpr noexcept(
	Trait::IsNothrowViewable<R>
	&& std::conjunction_v<
		std::is_nothrow_invocable<decltype(std::views::repeat), std::ranges::range_reference_t<R>, Size>,
		std::is_nothrow_invocable<decltype(std::ranges::size), R>
	>
) -> std::ranges::view auto {
	using std::ranges::size, std::views::repeat, std::cmp_greater_equal;

	const auto range_size = size(r);
	const Size pad_size = cmp_greater_equal(total_size, range_size) ? total_size - range_size : Size {};
	auto&& last = r.back();
	return Concat(std::forward<R>(r), repeat(std::forward<decltype(last)>(last), pad_size));
});

}