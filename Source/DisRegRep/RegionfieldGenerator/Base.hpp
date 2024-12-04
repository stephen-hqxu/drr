#pragma once

#include <DisRegRep/Container/Regionfield.hpp>

#include <random>
#include <string_view>

#include <cstdint>

namespace DisRegRep::RegionfieldGenerator {

/**
 * @brief Base of all regionfield generators.
*/
class Base {
public:

	using SeedType = std::uint32_t;

protected:

	using RandomIntType = std::uint16_t;
	using RandomEngineType =
		std::shuffle_order_engine<std::linear_congruential_engine<RandomIntType, 10621U, 210U, (1U << 15U) - 1U>, 16U>;
	using UniformDistributionType = std::uniform_int_distribution<RandomIntType>;

	/**
	 * @brief Create a new random engine using the currently set seed.
	 *
	 * @return Random engine with current seed.
	 */
	[[nodiscard]] RandomEngineType createRandomEngine() const noexcept;

	/**
	 * @brief Create a new distribution based on the region count specified in a regionfield.
	 *
	 * @param regionfield A regionfield whose distribution is to be created.
	 * 
	 * @return Uniform integer distribution with range [0, region count).
	 */
	[[nodiscard]] static UniformDistributionType createDistribution(const Container::Regionfield&) noexcept;

public:

	SeedType Seed {}; /**< A seed used by random number generators. */

	constexpr Base() noexcept = default;

	Base(const Base&) = delete;

	Base(Base&&) noexcept = delete;

	Base& operator=(const Base&) = delete;

	Base& operator=(Base&&) noexcept = delete;

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
	virtual void operator()(Container::Regionfield&) = 0;

};

}