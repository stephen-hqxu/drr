#pragma once

#include <DisRegRep/Container/Regionfield.hpp>
#include <DisRegRep/Core/XXHash.hpp>

#include <random>
#include <string_view>

#include <cstdint>

namespace DisRegRep::RegionfieldGenerator {

/**
 * @brief Base of all regionfield generators.
*/
class Base {
public:

	using SeedType = std::uint_fast32_t;

protected:

	using RandomIntType = std::uint_fast16_t;
	using UniformDistributionType = std::uniform_int_distribution<RandomIntType>;

	/**
	 * @brief Generate a secret sequence using the currently set seed.
	 *
	 * @return Secret sequence with current seed.
	 */
	[[nodiscard]] Core::XXHash::Secret generateSecret() const;

	/**
	 * @brief Create a new distribution based on the region count specified in a regionfield.
	 *
	 * @param regionfield A regionfield whose distribution is to be created.
	 * 
	 * @return Uniform integer distribution with range [0, region count).
	 */
	[[nodiscard]] static UniformDistributionType createDistribution(const Container::Regionfield&);

public:

	SeedType Seed {}; /**< A seed used by random number generators. */

	constexpr Base() noexcept = default;

	Base(const Base&) = delete;

	Base(Base&&) = delete;

	Base& operator=(const Base&) = delete;

	Base& operator=(Base&&) = delete;

	virtual constexpr ~Base() = default;

	/**
	 * @brief Get an identifying name for the regionfield generator.
	 *
	 * @return The generator name.
	 */
	[[nodiscard]] virtual constexpr std::string_view name() const noexcept = 0;

	/**
	 * @brief Generate regionfield.
	 *
	 * @param[Out] regionfield A regionfield matrix where generated contents are stored.
	 */
	virtual void operator()(Container::Regionfield&) const = 0;

};

}