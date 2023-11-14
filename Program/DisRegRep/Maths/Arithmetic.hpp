#pragma once

#include <array>
#include <iterator>

#include <execution>
#include <algorithm>
#include <ranges>
#include <numeric>

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
 * @brief Normalise `input` and store results to `output`.
 * 
 * @tparam RI Input range.
 * @tparam RO Output iterator.
 * 
 * @param input The input range to be normalised.
 * @param output The output iterator.
*/
template<std::ranges::input_range RI, std::weakly_incrementable O>
inline void normalise(RI&& input, O&& output) {
	using std::ranges::cbegin, std::ranges::cend, std::ranges::range_value_t, std::execution::unseq;
	
	const double sum = std::reduce(unseq, cbegin(input), cend(input));
	std::transform(unseq, cbegin(input), cend(input), output,
		[sum](const auto val) constexpr noexcept
			{ return static_cast<typename std::iterator_traits<O>::value_type>(val / sum); });
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

}

}