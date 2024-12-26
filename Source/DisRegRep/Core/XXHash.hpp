#pragma once

#include <array>
#include <span>

#include <algorithm>
#include <memory>
#include <tuple>

#include <limits>
#include <type_traits>
#include <utility>

#include <cstddef>
#include <cstdint>

/**
 * @brief Information digestion with *xxHash* algorithm.
 */
namespace DisRegRep::Core::XXHash {

inline constexpr std::uint8_t ApplicationSecretSize = 80U, /**< Size in byte of the secret specified by the end application. */
	TotalSecretSize = ApplicationSecretSize * 2U; /**< Size in byte of the total secret sequence. */

using SeedType = std::uint_fast64_t;
using HashType = std::uint64_t;

using ApplicationSecret = std::span<const std::byte, ApplicationSecretSize>;
using Secret = std::array<std::byte, TotalSecretSize>;
using SecretView = std::span<const std::byte, TotalSecretSize>;
using Input = std::span<const std::byte>;

/**
 * @brief Generate a secret sequence to be consumed by the hash function to predictably change the hash.
 *
 * @param app_secret A secret sequence specified by the application. This secret should provide sufficient entropy to allow an
 * unpredictable hash to be generated.
 * @param seed A random seed for generation of the new secret.
 *
 * @return The generated secret.
 */
Secret generateSecret(ApplicationSecret, SeedType);

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
HashType hash(Input, SecretView) noexcept;

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
HashType hash(const SecretView secret, const std::tuple<Obj...>& obj) noexcept {
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
 * @tparam Obj Object types.
 */
template <typename... Obj>
requires(sizeof...(Obj) > 0U && std::conjunction_v<std::is_trivially_copyable<Obj>...>)
class RandomEngine {
public:

	static constexpr auto ObjectSize = sizeof...(Obj);

	using ResultType = HashType;
	using ResultLimit = std::numeric_limits<ResultType>;

	using CounterType = std::uint_fast16_t;
	using CounterReference = std::add_lvalue_reference_t<CounterType>;
	using ObjectType = std::tuple<CounterType, Obj...>;

private:

	SecretView Secret;
	ObjectType Object;

public:

	/**
	 * @brief Construct a XXH engine.
	 *
	 * @tparam Another Object types.
	 *
	 * @param secret Secret sequence to control the engine. The engine shares the ownership of this secret until it is destroyed.
	 * @param another Objects to be hashed.
	 */
	template<typename... Another>
	requires(sizeof...(Another) == ObjectSize && std::conjunction_v<std::is_constructible<Obj, Another>...>)
	explicit(std::negation_v<std::conjunction<std::is_convertible<Another, Obj>...>>) RandomEngine(
		const SecretView secret, Another&&... another) noexcept(std::conjunction_v<std::is_nothrow_constructible<Obj, Another>...>) :
		Secret(secret), Object(CounterType {}, std::forward<Another>(another)...) { }

	constexpr ~RandomEngine() = default;

	[[nodiscard]] ResultType operator()() noexcept {
		const ResultType number = hash(this->Secret, this->Object);
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
	[[nodiscard]] constexpr std::conditional_t<std::is_const_v<Self>, CounterType, CounterReference> counter(
		this Self& self) noexcept {
		return std::get<0U>(self.Object);
	}

};

template<typename... Obj>
RandomEngine(SecretView, Obj...) -> RandomEngine<Obj...>;

}