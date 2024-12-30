#pragma once

#include "Range.hpp"

#include <glm/vec2.hpp>

#include <functional>
#include <ranges>

#include <concepts>
#include <type_traits>
#include <utility>

#ifdef DRR_CORE_ARITHMETIC_DEPRECATED
#include "Range.hpp"
#include "Type.hpp"

#include <glm/fwd.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iterator>
#include <span>

#include <algorithm>
#include <execution>
#include <functional>
#include <ranges>

#include <utility>
#include <type_traits>
#include <concepts>

#include <cstddef>
#include <cstdint>
#endif//DRR_CORE_ARITHMETIC_DEPRECATED

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
		| transform([offset_x = offset.x, extent_x = extent.x](auto inner_r) constexpr noexcept(
			std::is_nothrow_move_constructible_v<InnerR>
		) {
			return std::move(inner_r)
				| drop(offset_x)
				| take(extent_x);
		});
});

#ifdef DRR_CORE_ARITHMETIC_DEPRECATED
/**
 * @brief Convert an integer to signed.
 *
 * @tparam I Integer type.
 * @param i Integer to be converted to signed.
 *
 * @return Signed version of `i` if it is unsigned; otherwise unmodified.
 */
template<std::integral I>
[[deprecated, nodiscard]] constexpr auto toSigned(const I i) noexcept {
	return static_cast<std::make_signed_t<I>>(i);
}

/**
 * @brief Convert an integral vector to signed.
 *
 * @tparam L Vector length.
 * @tparam T Vector type.
 * @tparam Q Vector qualifier.
 *
 * @param v Vector to be converted to signed.
 *
 * @return Signed version of `v` if it is unsigned; otherwise unmodified.
 */
template<glm::length_t L, std::integral T, glm::qualifier Q>
[[deprecated("Use GLM vector implicit conversion."), nodiscard]]
constexpr auto toSigned(const glm::vec<L, T, Q>& v) {
	using glm::value_ptr;
	using std::span, std::transform, std::execution::unseq;

	glm::vec<L, std::make_signed_t<T>, Q> signed_v;
	const auto it = span<T, L>(value_ptr(v), L);
	transform(unseq, it.cbegin(), it.cend(), value_ptr(signed_v), toSigned<T>);
	return signed_v;
}

/**
 * @brief Convert an integer to unsigned.
 * 
 * @tparam I Integer type.
 * @param i Integer to be converted to unsigned.
 * 
 * @return Unsigned version of `i` if it is signed; otherwise unmodified.
 */
template<std::integral I>
[[deprecated, nodiscard]] constexpr auto toUnsigned(const I i) noexcept {
	return static_cast<std::make_unsigned_t<I>>(i);
}

/**
 * @brief Perform `output = f(a, b)`.
 *
 * @param a The first range.
 * @param f The range operator.
 * @param b The second range.
 * @param output The output iterator.
 */
[[deprecated("Redundant; just use plain transform.")]] void addRange(
	std::ranges::forward_range auto&& a,
	std::move_constructible auto f,
	std::ranges::forward_range auto&& b,
	std::forward_iterator auto output
) {
	using std::transform, std::execution::unseq,
		std::ranges::cbegin, std::ranges::cend;

	transform(unseq, cbegin(a), cend(a), cbegin(b), std::move(output), std::move(f));
}

/**
 * @brief Scale each value in the range.
 * 
 * @tparam R The input range type.
 * @tparam O The output iterator type.
 * @tparam T The scalar type.
 * 
 * @param input The input range.
 * @param output The output iterator.
 * @param scalar The scalar.
*/
template<
	std::floating_point T,
	std::ranges::input_range R,
	std::output_iterator<T> O
>
requires std::is_convertible_v<std::ranges::range_value_t<R>, T>
[[deprecated("Superseded by Arithmetic::Normalise.")]]
void scaleRange(R&& input, const O output, const T scalar) {
	using std::transform, std::execution::unseq,
		std::ranges::cbegin, std::ranges::cend, std::iterator_traits;

	using OutputType = typename iterator_traits<O>::value_type;
	transform(unseq, cbegin(input), cend(input), output,
		[factor = T { 1 } / scalar](const OutputType v) constexpr noexcept { return v * factor; });
}

/**
 * @brief Calculate product of all numbers in a vector.
 *
 * @tparam L Vector length.
 * @tparam T Vector type.
 * @tparam Q Vector qualifier.
 *
 * @param v Input vector whose horizontal product is to be calculated.
 *
 * @return The horizontal product of `v`.
*/
template<glm::length_t L, typename T, glm::qualifier Q>
[[deprecated("Previously used for computing multi-dimensional strides, now superseded by mdspan mapping."), nodiscard]]
constexpr T horizontalProduct(const glm::vec<L, T, Q>& v) noexcept {
	using std::index_sequence, std::make_index_sequence;
	const auto product = [&v]<std::size_t... I>(index_sequence<I...>) constexpr noexcept -> T {
		return (T { 1 } * ... * v[I]);
	};
	return product(make_index_sequence<L> {});
}

/**
 * @brief Calculate the kernel area given radius.
 * 
 * @tparam T The type of radius.
 * @param radius The radius of kernel.
 * 
 * @return The kernel area.
*/
[[deprecated("Calculation of kernel area is an implementation detail of the splatting convolution; keep it private."), nodiscard]]
constexpr std::uint32_t kernelArea(const std::uint32_t radius) noexcept {
	const std::uint32_t diameter = 2U * radius + 1U;
	return diameter * diameter;
}
#endif//DRR_CORE_ARITHMETIC_DEPRECATED

}