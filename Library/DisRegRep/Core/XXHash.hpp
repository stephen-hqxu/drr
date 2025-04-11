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

inline constexpr std::uint_fast8_t ApplicationSecretSize = 80U, /**< Size in byte of the secret specified by the end application. */
	TotalSecretSize = ApplicationSecretSize * 2U; /**< Size in byte of the total secret sequence. */

using SeedType = std::uint_fast64_t;
using HashType = std::uint64_t;

using ApplicationSecret = std::array<std::byte, ApplicationSecretSize>;
using ApplicationSecretView = std::span<const std::byte, ApplicationSecretSize>;
using Secret = std::array<std::byte, TotalSecretSize>;
using SecretView = std::span<const std::byte, TotalSecretSize>;
using Input = std::span<const std::byte>;

/**
 * @brief Create an application secret array from a string of space-separated base-16 bytes.
 *
 * @param str String that contains the secret bytes.
 *
 * @return An array of application secret.
 */
[[nodiscard]] consteval ApplicationSecret makeApplicationSecret(const std::string_view str) {
	using std::ranges::transform,
		std::views::split, std::ranges::data, std::ranges::size,
		std::from_chars, std::errc;

	//Each byte has two hex characters plus a space separator, hence there are three characters per token;
	//	the final token does not have a separator, hence minus one.
	DRR_ASSERT(str.length() == ApplicationSecretSize * 3UZ - 1UZ);

	ApplicationSecret secret {};
	transform(str | split(' '), secret.begin(), [](const auto token) static consteval {
		std::uint8_t byte {};
		const auto first = data(token);
		DRR_ASSERT(from_chars(first, first + size(token), byte, 16).ec == errc {});
		return std::byte { byte };
	});
	return secret;
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
requires(sizeof...(Obj) > 0U && std::conjunction_v<std::is_trivially_copyable<Obj>...>)
[[nodiscard]] HashType hash(const SecretView secret, const std::tuple<Obj...>& obj) noexcept {
	using std::array, std::apply, std::ranges::copy_n;
	using std::addressof, std::remove_reference_t;

	//Do not find the tuple size directly as it may contain implementation-specific padding.
	static constexpr std::size_t ObjectSize = (sizeof(remove_reference_t<Obj>) + ...);

	return hash(
		apply([]<typename... O>(const O&... o) static noexcept {
			array<std::byte, ObjectSize> input_binary;
			auto begin = input_binary.begin();
			((begin = copy_n(reinterpret_cast<const std::byte*>(addressof(o)), sizeof(O), begin).out), ...);
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
requires std::is_constructible_v<SecretView, Sec> && std::conjunction_v<std::is_trivially_copyable<Obj>...>
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
	RandomEngine(AnotherSec&& secret, AnotherObj&&... object)
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
RandomEngine(Sec&&, Obj...) -> RandomEngine<std::views::all_t<Sec>, Obj...>;

}