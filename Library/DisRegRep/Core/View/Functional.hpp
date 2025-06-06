#pragma once

#include "RangeAdaptorClosure.hpp"
#include "Trait.hpp"

#include <tuple>

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
	[[nodiscard]] static constexpr std::ranges::view auto operator()(R&& r) noexcept(Trait::IsNothrowViewable<R>) {
		using std::views::transform;
		return std::forward<R>(r)
			 | transform([]<typename Value>(Value&& value) static constexpr noexcept(
							 std::is_nothrow_convertible_v<Value, To>) -> decltype(auto) {
				   return static_cast<To>(std::forward<Value>(value));
			   });
	}

};

template<typename T>
struct MakeFromTuple {
private:

	template<typename>
	struct Argument;
	template<typename... Arg>
	struct Argument<std::tuple<Arg...>> {

		static constexpr bool Constructible = std::is_constructible_v<T, Arg...>,
			NothrowConstructible = std::is_nothrow_constructible_v<T, Arg...>;

	};
	template<std::ranges::range R>
	using RangeArgument = Argument<std::ranges::range_reference_t<R>>;

public:

	template<std::ranges::viewable_range R>
	requires std::ranges::input_range<R> && RangeArgument<R>::Constructible
	[[nodiscard]] static constexpr std::ranges::view auto operator()(R&& r) noexcept(Trait::IsNothrowViewable<R>) {
		using std::views::transform, std::make_from_tuple;
		return std::forward<R>(r)
			 | transform([]<typename Tuple>(Tuple&& tuple) static constexpr noexcept(
							 RangeArgument<R>::NothrowConstructible) { return make_from_tuple<T>(std::forward<Tuple>(tuple)); });
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
	(R&& r) static constexpr noexcept(Trait::IsNothrowViewable<R>) -> std::ranges::view auto {
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
		(R&& r) static constexpr noexcept(Trait::IsNothrowViewable<R>) -> std::ranges::view auto {
			using std::views::transform;
			return std::forward<R>(r)
				 | transform([]<typename V>(V&& v) static noexcept(
								 noexcept(*std::declval<Value>())) -> decltype(auto) { return *std::forward<V>(v); });
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

/**
 * @brief Construct a range of objects using each range tuple element as arguments to the constructor.
 *
 * @note Unlike @link std::make_from_tuple, it only supports if range value is a tuple; other tuple-like types are unsupported.
 *
 * @tparam T Type to be constructed.
 */
template<typename T>
inline constexpr auto MakeFromTuple = RangeAdaptorClosure(Internal_::MakeFromTuple<T> {});

}