#pragma once

#include "IrregularTransform.hpp"
#include "RangeAdaptorClosure.hpp"

#include <functional>
#include <ranges>

#include <utility>

#include <concepts>
#include <type_traits>

#include <cstdint>

namespace DisRegRep::Core::View {

/**
 * @brief Source range values by repeatedly invoking a given generator function.
 *
 * @tparam Generator Type of generator. The generator does not need to be regular.
 *
 * @param generator A function that takes no argument and returns a value that becomes the resulting range value on invoke.
 * @param n Size of the generated range.
 *
 * @return Generated range.
 */
inline constexpr auto Generate = []<RangeApplicator Generator>
requires std::is_invocable_v<Generator>
(Generator generator, const std::unsigned_integral auto n) static constexpr noexcept(
	std::is_nothrow_move_constructible_v<Generator>) -> std::ranges::view auto {
	using std::invoke, std::views::repeat, std::is_nothrow_invocable_v;
	return repeat(std::uint_fast8_t {}, n)
		 | IrregularTransform([generator = std::move(generator)](auto) mutable constexpr noexcept(
								  is_nothrow_invocable_v<Generator>) { return invoke(generator); });
};

}