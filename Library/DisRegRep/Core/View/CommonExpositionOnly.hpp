#pragma once

#include <type_traits>

/**
 * @brief Commonly used exposition-only definitions for implementing standard-conforming components.
 */
namespace DisRegRep::Core::View::CommonExpositionOnly {

//Forward rvalue `t` as lvalue.
template<class T>
[[nodiscard]] constexpr T& asLValue(T&& t) noexcept {//NOLINT(cppcoreguidelines-missing-std-forward)
	return static_cast<std::add_lvalue_reference_t<std::remove_reference_t<T>>>(t);
}

}