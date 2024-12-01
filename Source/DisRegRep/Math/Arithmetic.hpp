#pragma once

#include <DisRegRep/Type.hpp>

#include <glm/fwd.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iterator>
#include <span>

#include <execution>
#include <algorithm>
#include <ranges>

#include <utility>
#include <type_traits>
#include <concepts>

#include <cstddef>
#include <cstdint>

/**
 * @brief Standard algebraic operations.
 */
namespace DisRegRep::Math::Arithmetic {

/**
 * @brief Convert an integer to signed.
 *
 * @tparam I Integer type.
 * @param i Integer to be converted to signed.
 *
 * @return Signed version of `i` if it is unsigned; otherwise unmodified.
 */
template<std::integral I>
[[nodiscard]] constexpr auto toSigned(const I i) noexcept {
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
[[nodiscard]] constexpr auto toSigned(const glm::vec<L, T, Q>& v) {
	using glm::value_ptr;
	using std::span, std::transform, std::execution::unseq;

	glm::vec<L, std::make_signed_t<T>, Q> signed_v;
	const auto it = span(value_ptr(v), L);
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
[[nodiscard]] constexpr auto toUnsigned(const I i) noexcept {
	return static_cast<std::make_unsigned_t<I>>(i);
}

/**
 * @brief Perform `output = f(a, b)`.
 * 
 * @tparam R1 The range type of `a`.
 * @tparam F The type of operator.
 * @tparam R2 The range type of `b`.
 * @tparam O The output iterator type.
 * 
 * @param a The first range.
 * @param f The range operator.
 * @param b The second range.
 * @param output The output iterator.
*/
template<
	std::ranges::input_range R1,
	std::invocable F,
	std::ranges::input_range R2,
	std::output_iterator<std::indirect_result_t<F, std::ranges::range_reference_t<R1>, std::ranges::range_reference_t<R2>>> O
>
void addRange(const R1& a, F&& f, const R2& b, const O output) {
	using std::transform, std::execution::unseq,
		std::ranges::cbegin, std::ranges::cend;

	transform(unseq, cbegin(a), cend(a), cbegin(b), output, std::forward<F>(f));
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
void scaleRange(const R& input, const O output, const T scalar) {
	using std::transform, std::execution::unseq,
		std::ranges::cbegin, std::ranges::cend, std::iterator_traits;

	transform(unseq, cbegin(input), cend(input), output, [factor = T { 1 } / scalar](const auto v) constexpr noexcept {
		return static_cast<typename iterator_traits<O>::value_type>(v * factor);
	});
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
[[nodiscard]] constexpr T horizontalProduct(const glm::vec<L, T, Q>& v) noexcept {
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
[[nodiscard]] constexpr std::uint32_t kernelArea(const Type::Radius radius) noexcept {
	const std::uint32_t diameter = 2U * radius + 1U;
	return diameter * diameter;
}

}//namespace DisRegRep::Math::Arithmetic