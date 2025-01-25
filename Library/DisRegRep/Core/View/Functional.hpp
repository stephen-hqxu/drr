#pragma once

#include "RangeAdaptorClosure.hpp"

#include <ranges>

#include <memory>
#include <utility>

#include <type_traits>

/**
 * @brief Apply a function object to a range.
 */
namespace DisRegRep::Core::View::Functional {

namespace Internal_ {

template<typename To>
struct Cast {

	template<std::ranges::viewable_range R>
	requires std::ranges::input_range<R> && std::is_convertible_v<std::ranges::range_reference_t<R>, To>
	[[nodiscard]] static constexpr std::ranges::view auto operator()(R&& r)
		noexcept(std::is_nothrow_constructible_v<std::remove_cvref_t<R>, R>) {
		using std::views::transform;
		return std::forward<R>(r)
			 | transform([]<typename Value>(Value&& value) static constexpr noexcept(
							 std::is_nothrow_convertible_v<Value, To>) { return static_cast<To>(std::forward<Value>(value)); });
	}

};

}

/**
 * @brief Take the address of each range value.
 *
 * @tparam R Range type.
 *
 * @param r Input range whose elements' addresses are taken.
 *
 * @return Address of each element of `r`.
 */
inline constexpr auto AddressOf = RangeAdaptorClosure([]<std::ranges::viewable_range R>
	requires std::ranges::input_range<R> && std::is_lvalue_reference_v<std::ranges::range_reference_t<R>>
	(R&& r) static constexpr noexcept(std::is_nothrow_constructible_v<std::remove_cvref_t<R>, R>) -> std::ranges::view auto {
		using std::views::transform, std::addressof;
		return std::forward<R>(r) | transform([](auto& obj) static constexpr noexcept { return addressof(obj); });
	});

/**
 * @brief Dereference each value in the range.
 *
 * @tparam R Range type.
 *
 * @param r Input range whose elements are dereferenced.
 *
 * @return Dereferenced results of each element in `r`.
 */
inline constexpr auto Dereference =
	RangeAdaptorClosure([]<std::ranges::viewable_range R, typename Value = std::ranges::range_value_t<R>>
		requires std::ranges::input_range<R> && requires(Value value) { *value; }
		(R&& r) static constexpr noexcept(std::is_nothrow_constructible_v<std::remove_cvref_t<R>, R>) -> std::ranges::view auto {
			using std::views::transform;
			return std::forward<R>(r)
				 | transform([]<typename V>(V&& v) static noexcept(noexcept(*std::declval<Value>())) { return *std::forward<V>(v); });
		});

/**
 * @brief Cast range type to another type.
 *
 * @tparam To Target type.
 *
 * @param r Input range whose elements are cast.
 *
 * @return Type conversion applied to each element in `r`.
 */
template<typename To>
inline constexpr auto Cast = RangeAdaptorClosure(Internal_::Cast<To> {});

}