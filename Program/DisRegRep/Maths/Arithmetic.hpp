#pragma once

#include <array>
#include <iterator>

#include <execution>
#include <algorithm>
#include <ranges>
#include <numeric>

#include <utility>
#include <type_traits>
#include <concepts>

namespace DisRegRep {

/**
 * @brief Just some helps to do fundamental calculations.
*/
namespace Arithmetic {

using ssize_t = std::make_signed_t<std::size_t>;/**< signed size_t */

template<std::integral I>
constexpr auto toSigned(const I num) noexcept {
	return static_cast<std::make_signed_t<I>>(num);
}
template<std::integral T, size_t S>
constexpr auto toSigned(const std::array<T, S>& nums) {
	std::array<std::make_signed_t<T>, S> snums { };
	std::ranges::transform(nums, snums.begin(), [](const auto v) constexpr noexcept { return toSigned(v); });
	return snums;
}

template<std::integral I>
constexpr auto toUnsigned(const I num) noexcept {
	return static_cast<std::make_unsigned_t<I>>(num);
}

/**
 * @brief Perform `output` = `a` `op` `b`.
 * 
 * @tparam Op The type of operator.
 * @tparam R1 The range type of `a`.
 * @tparam R2 The range type of `b`.
 * @tparam RO The output iterator type.
 * 
 * @param a The first range.
 * @param op The range operator.
 * @param b The second range.
 * @param output The output iterator.
*/
template<typename Op, std::ranges::input_range R1, std::ranges::input_range R2,
	std::weakly_incrementable O>
inline void addRange(R1&& a, Op&& op, R2&& b, O&& output) {
	using std::ranges::cbegin, std::ranges::cend;
	
	std::transform(std::execution::unseq, cbegin(a), cend(a), cbegin(b), output, std::forward<Op>(op));
}

/**
 * @brief Scale each value in the range.
 * 
 * @tparam R The input range type.
 * @tparam O The output range type.
 * @tparam T The scalar type.
 * @param input The input range.
 * @param output The output range.
 * @param scalar The scalar.
*/
template<std::floating_point T, std::ranges::input_range R, std::weakly_incrementable O>
inline void scaleRange(R&& input, O&& output, const T scalar) {
	using std::ranges::cbegin, std::ranges::cend, std::iterator_traits;

	std::transform(std::execution::unseq, cbegin(input), cend(input), output,
		[scalar](const auto v) constexpr noexcept
			{ return static_cast<iterator_traits<O>::value_type>(v / scalar); });
}

/**
 * @brief Calculate product of all numbers in an array.
 * 
 * @tparam T Type of number.
 * @tparam S The size of array.
 * @param r The range whose horizontal product is calculated.
*/
template<typename T, size_t S>
constexpr T horizontalProduct(const std::array<T, S>& num) noexcept {
	const auto product = [&num]<size_t... Idx>(std::index_sequence<Idx...>) noexcept -> T {
		return (T { 1 } * ... * num[Idx]);
	};
	return product(std::make_index_sequence<S> { });
}

}

}