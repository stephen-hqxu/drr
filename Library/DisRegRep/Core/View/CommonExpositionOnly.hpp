#pragma once

#include <ranges>

#include <concepts>
#include <type_traits>

/**
 * @brief Commonly used exposition-only definitions for implementing standard-conforming components.
 */
namespace DisRegRep::Core::View::CommonExpositionOnly {

//Conditionally apply a `const` qualifier to the type `T`.
template<bool Const, typename T>
using MaybeConst = std::conditional_t<Const, std::add_const_t<T>, T>;

template<typename R>
concept SimpleView = std::ranges::view<R> && std::ranges::range<std::add_const_t<R>>
				  && std::same_as<std::ranges::iterator_t<R>, std::ranges::iterator_t<std::add_const_t<R>>>
				  && std::same_as<std::ranges::sentinel_t<R>, std::ranges::sentinel_t<std::add_const_t<R>>>;

//Forward rvalue `t` as lvalue.
template<typename T>
[[nodiscard]] constexpr T& asLValue(T&& t) noexcept {//NOLINT(cppcoreguidelines-missing-std-forward)
	return static_cast<std::add_lvalue_reference_t<std::remove_reference_t<T>>>(t);
}

}