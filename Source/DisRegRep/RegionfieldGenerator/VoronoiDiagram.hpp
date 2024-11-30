#pragma once

#include "RegionMapFactory.hpp"

#include <cstdint>

namespace DisRegRep {

/**
 * @brief Generate a region map organised as a voronoi diagram.
*/
class VoronoiRegionFactory final : public RegionMapFactory {
public:

	std::uint64_t RandomSeed;/**< Seed used by random number generator */
	size_t CentroidCount; /**< The number of centroid. */

	explicit constexpr VoronoiRegionFactory(const std::uint64_t seed = 0u, const size_t centroid_count = 0u) noexcept :
		RandomSeed(seed),
		CentroidCount(centroid_count) { }

	constexpr ~VoronoiRegionFactory() override = default;

	constexpr std::string_view name() const noexcept override {
		return "Voronoi";
	}

	void operator()(const CreateDescription&, RegionMap&) const override;

};

}