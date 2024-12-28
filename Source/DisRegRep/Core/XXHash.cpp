#include <DisRegRep/Core/XXHash.hpp>

#include <xxhash.h>

#include <array>
#include <span>

#include <algorithm>
#include <functional>

#include <random>

#include <type_traits>

#include <cstdint>

namespace XXHash = DisRegRep::Core::XXHash;

using std::array, std::span;
using std::ranges::generate, std::ranges::transform,
	std::bit_xor;
using std::mt19937_64;

static_assert(XXHash::TotalSecretSize >= XXH3_SECRET_SIZE_MIN);
static_assert(std::is_same_v<XXHash::HashType, XXH64_hash_t>);

XXHash::Secret XXHash::generateSecret(const ApplicationSecretView app_secret, const SeedType seed) {
	using GeneratedType = std::uint64_t;
	static_assert(ApplicationSecretSize % sizeof(GeneratedType) == 0U);
	static_assert(TotalSecretSize % sizeof(GeneratedType) == 0U);
	static constexpr std::uint8_t TypedTotalSecretSize = TotalSecretSize / sizeof(GeneratedType);

	//Only Mersenne-Twister engine guarantees that output range covers the whole bit range without giving bias.
	Secret secret;
	//Will not violate strict aliasing rule because `GeneratedType` is the only alive type besides std::byte.
	const auto typed_secret =
		span<GeneratedType, TypedTotalSecretSize>(reinterpret_cast<GeneratedType*>(secret.data()), TypedTotalSecretSize);
	generate(typed_secret, mt19937_64(seed));
	transform(app_secret, secret, secret.begin(), bit_xor {});
	return secret;
}

XXHash::HashType XXHash::hash(const Input input, const SecretView secret) noexcept {
	return XXH3_64bits_withSecret(input.data(), input.size_bytes(), secret.data(), secret.size_bytes());
}