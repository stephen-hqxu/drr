#pragma once

#include "RegionMapFactory.hpp"

#include <cstdint>

namespace DisRegRep {

/**
 * @brief Create a region map with random number generator.
*/
class RandomRegionFactory : public RegionMapFactory {
public:

	std::uint32_t RandomSeed;/**< Set random seed to be effected in the next generation call. */

	constexpr RandomRegionFactory(const std::uint32_t seed = 0u) noexcept : RandomSeed(seed) {

	}

	constexpr ~RandomRegionFactory() override = default;

	Format::RegionMap operator()(const RegionMapFactory::CreateDescription& desc) const override;

};

}