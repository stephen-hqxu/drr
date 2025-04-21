#pragma once

#include "View/Trait.hpp"
#include "Exception.hpp"

#include <array>
#include <span>

#include <string_view>
#include <tuple>

#include <algorithm>
#include <ranges>

#include <charconv>
#include <memory>
#include <utility>

#include <limits>
#include <type_traits>

#include <cstddef>
#include <cstdint>

/**
 * @brief Information digestion with *xxHash* algorithm.
 */
namespace DisRegRep::Core::XXHash {

/**
 * @brief Check if `T` can be hashed by the XXHash algorithm.
 */
template<typename T>
concept Hashable = std::is_trivially_copyable_v<std::remove_cvref_t<T>>;

inline constexpr std::uint_fast8_t ApplicationSecretSize = 80U, /**< Size in byte of the secret specified by the end application. */
	TotalSecretSize = ApplicationSecretSize * 2U; /**< Size in byte of the total secret sequence. */

using SeedType = std::uint_fast64_t;
using HashType = std::uint64_t;

using ApplicationSecret = std::array<std::byte, ApplicationSecretSize>;
using ApplicationSecretView = std::span<const std::byte, ApplicationSecretSize>;
using Secret = std::array<std::byte, TotalSecretSize>;
using SecretView = std::span<const std::byte, TotalSecretSize>;
using Input = std::span<const std::byte>;

namespace Internal_ {

template<Hashable Obj>
inline constexpr std::size_t ObjectSize = sizeof(std::remove_reference_t<Obj>); /**< Get byte size of a hashable object. */
template<Hashable T, std::size_t Size>
inline constexpr std::size_t ObjectSize<std::array<T, Size>> = ObjectSize<T> * Size;

/**
 * @brief Get address to a hashable object.
 *
 * @param obj A hashable object.
 *
 * @return Address of `obj`.
 */
[[nodiscard]] constexpr const std::byte* objectAddress(const Hashable auto& obj) noexcept {
	return reinterpret_cast<const std::byte*>(std::addressof(obj));
}
template<Hashable T, std::size_t Size>
[[nodiscard]] constexpr const std::byte* objectAddress(const std::array<T, Size>& obj) noexcept {
	return std::as_bytes(std::span(obj)).data();
}

}

/**
 * @brief Create a secret array from a string of space-separated base-16 bytes.
 *
 * @tparam Size Secret size in bytes.
 *
 * @param str String that contains the secret bytes.
 *
 * @return An array of secrets.
 */
template<std::size_t Size>
[[nodiscard]] consteval std::array<std::byte, Size> makeSecret(const std::string_view str) {
	using std::ranges::transform,
		std::views::split, std::ranges::data, std::ranges::size,
		std::from_chars, std::errc;

	//Each byte has two hex characters plus a space separator, hence there are three characters per token;
	//	the final token does not have a separator, hence minus one.
	DRR_ASSERT(str.length() == Size * 3UZ - 1UZ);

	std::array<std::byte, Size> secret {};
	transform(str | split(' '), secret.begin(), [](const auto token) static consteval {
		std::uint8_t byte {};
		const auto first = data(token);
		DRR_ASSERT(from_chars(first, first + size(token), byte, 16).ec == errc {});
		return std::byte { byte };
	});
	return secret;
}
/**
 * @brief Create an application secret array with @link makeSecret.
 */
[[nodiscard]] consteval ApplicationSecret makeApplicationSecret(const std::string_view str) {
	return makeSecret<ApplicationSecretSize>(str);
}

/**
 * @brief Generate a secret sequence to be consumed by the hash function to predictably change the hash.
 *
 * @param app_secret A secret sequence specified by the application. This secret should provide sufficient entropy to allow an
 * unpredictable hash to be generated.
 * @param seed A random seed for generation of the new secret.
 *
 * @return The generated secret.
 */
[[nodiscard]] Secret generateSecret(ApplicationSecretView, SeedType);

/**
 * @brief Hash the input.
 *
 * @param input Input data to be hashed.
 * @param secret Secret sequence generated.
 *
 * @link generateSecret
 *
 * @return 64-bit XXH3 hash of `input` with `secret`.
 */
[[nodiscard]] HashType hash(Input, SecretView) noexcept;

/**
 * @brief Hash all objects.
 *
 * @tparam Obj Object types.
 *
 * @param secret Secret sequence generated.
 * @param obj Objects to be hashed.
 *
 * @return 64-bit XXH3 hash of all `obj`s with `secret`.
 */
template<typename... Obj>
requires(sizeof...(Obj) > 0U && (Hashable<Obj> && ...))
[[nodiscard]] HashType hash(const SecretView secret, const std::tuple<Obj...>& obj) noexcept {
	using std::array, std::apply, std::ranges::copy_n;
	//Do not find the tuple size directly as it may contain implementation-specific padding.
	static constexpr std::size_t ObjectSize = (Internal_::ObjectSize<Obj> + ...);

	return hash(
		apply([]<typename... O>(const O&... o) static noexcept {
			array<std::byte, ObjectSize> input_binary;
			auto begin = input_binary.begin();
			((begin = copy_n(Internal_::objectAddress(o), Internal_::ObjectSize<O>, begin).out), ...);
			return input_binary;
		}, obj),
		secret
	);
}

/**
 * @brief An adaptor that satisfies the requirements of being a random bit generator. This allows generation of random numbers with XXH
 * hash function. It is implemented as a counter-based engine, allowing cheap construction and parallel random number generation.
 *
 * @tparam Sec Secret type.
 * @tparam Obj Object types.
 */
template<std::ranges::view Sec, typename... Obj>
requires std::is_constructible_v<SecretView, Sec> && (Hashable<Obj> && ...)
class RandomEngine {
public:

	static constexpr auto ObjectSize = sizeof...(Obj);

	using SecretType = Sec;

	using ResultType = HashType;
	using ResultLimit = std::numeric_limits<ResultType>;

	using CounterType = std::uint_fast32_t;
	using CounterReference = std::add_lvalue_reference_t<CounterType>;
	using ObjectType = std::tuple<CounterType, Obj...>;

private:

	SecretType Secret_;
	ObjectType Object;

public:

	/**
	 * @brief Construct a XXH engine.
	 *
	 * @tparam AnotherSec Secret type.
	 * @tparam AnotherObj Object types.
	 *
	 * @param secret Secret sequence to control the engine. The ownership is correctly deduced based on its value category.
	 * @param object Objects to be hashed.
	 */
	template<std::ranges::viewable_range AnotherSec, typename... AnotherObj>
	requires(sizeof...(AnotherObj) == ObjectSize && std::conjunction_v<std::is_constructible<Obj, AnotherObj>...>)
	explicit RandomEngine(AnotherSec&& secret, AnotherObj&&... object)
		noexcept(View::Trait::IsNothrowViewable<AnotherSec> && std::conjunction_v<std::is_nothrow_constructible<Obj, AnotherObj>...>) :
		Secret_(std::forward<AnotherSec>(secret) | std::views::all), Object(CounterType {}, std::forward<AnotherObj>(object)...) { }

	[[nodiscard]] ResultType operator()() noexcept {
		//Static extent makes span explicitly constructed.
		const ResultType number = hash(SecretView(this->Secret_), this->Object);
		++this->counter();
		return number;
	}

	[[nodiscard]] static consteval ResultType min() noexcept {
		return ResultLimit::min();
	}

	[[nodiscard]] static consteval ResultType max() noexcept {
		return ResultLimit::max();
	}

	/**
	 * @brief Get the internal counter of the XXH engine.
	 *
	 * @return The current counter.
	 */
	template<typename Self>
	[[nodiscard]] constexpr std::conditional_t<std::disjunction_v<
		std::is_rvalue_reference<Self&&>,
		std::is_const<Self>
	>, CounterType, CounterReference> counter(this Self&& self) noexcept {
		return std::get<0U>(self.Object);
	}

};

//Takes ownership of `Sec` whenever necessary.
template<typename Sec, typename... Obj>
RandomEngine(Sec&&, Obj&&...) -> RandomEngine<std::views::all_t<Sec>, Obj...>;

}