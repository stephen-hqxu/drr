#pragma once

#include "RegionMapFactory.hpp"

#include <cstdint>

namespace DisRegRep {

/**
 * @brief Create a region map with random number generator.
*/
class RandomRegionFactory final : public RegionMapFactory {
public:

	std::uint64_t RandomSeed; /**< Set random seed to be effected in the next generation call. */

	explicit constexpr RandomRegionFactory(const std::uint64_t seed = 0u) noexcept : RandomSeed(seed) { }

	constexpr ~RandomRegionFactory() override = default;

	constexpr std::string_view name() const noexcept override {
		return "Random";
	}

	void operator()(const CreateDescription&, RegionMap&) const override;

};

}