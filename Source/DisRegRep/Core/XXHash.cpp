#include <DisRegRep/Core/XXHash.hpp>

#include <xxhash.h>

#include <array>
#include <span>

#include <algorithm>
#include <functional>

#include <random>

#include <type_traits>

namespace XXHash = DisRegRep::Core::XXHash;

using std::array, std::span, std::as_bytes;
using std::ranges::generate, std::ranges::transform, std::ranges::copy,
	std::bit_xor;
using std::mt19937_64;

static_assert(XXHash::TotalSecretSize >= XXH3_SECRET_SIZE_MIN);
static_assert(std::is_same_v<XXHash::HashType, XXH64_hash_t>);

XXHash::Secret XXHash::generateSecret(const ApplicationSecret app_secret, const SeedType seed) {
	static constexpr auto RandomSequenceSize = TotalSecretSize / sizeof(mt19937_64::result_type);

	array<mt19937_64::result_type, RandomSequenceSize> random_seq;
	generate(random_seq, mt19937_64(seed));
	const auto random_seq_byte = as_bytes(span(random_seq));

	Secret secret;
	const auto [_, random_seq_byte_it, secret_it] = transform(app_secret, random_seq_byte, secret.begin(), bit_xor {});
	copy(random_seq_byte_it, random_seq_byte.cend(), secret_it);
	return secret;
}

XXHash::HashType XXHash::hash(const Input input, const SecretView secret) noexcept {
	return XXH3_64bits_withSecret(input.data(), input.size_bytes(), secret.data(), secret.size_bytes());
}