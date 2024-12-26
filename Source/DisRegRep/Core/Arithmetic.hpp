#pragma once

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
 * @tparam V Normalising value type.
 *
 * @param r Input range of values.
 * @param factor Normalising factor. Each value in the range is multiplied by a reciprocal of this.
 *
 * @return Normalised range.
 */
inline constexpr auto Normalise = Range::RangeAdaptorClosure([]<std::ranges::viewable_range R, std::floating_point V>
	requires std::ranges::input_range<R> && std::is_convertible_v<std::ranges::range_value_t<R>, V>
	(R&& r, const V factor) static constexpr noexcept -> auto {
		using std::views::repeat, std::views::zip_transform, std::multiplies;
		return zip_transform(multiplies {}, std::forward<R>(r), repeat(V { 1 } / factor));
	});

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

}