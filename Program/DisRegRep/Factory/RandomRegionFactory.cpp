#include <DisRegRep/Factory/RandomRegionFactory.hpp>

#include <random>
#include <algorithm>

using std::ranlux24, std::uniform_int_distribution;
using std::ranges::generate;

using namespace DisRegRep;
using namespace Format;

RegionMap RandomRegionFactory::operator()(const RegionMapFactory::CreateDescription& desc) const {
	const auto& [dimension, region_count] = desc;
	const auto [x, y] = dimension;
	RegionMap map {
		.Map = FixedHeapArray<Region_t>(x * y),
		.Dimension = dimension,
		.RegionCount = region_count
	};

	generate(map.Map, [rng = ranlux24(static_cast<std::uint_fast32_t>(this->RandomSeed)),
		dist = uniform_int_distribution<std::uint64_t>(0u, region_count - 1u)]() mutable {
		return static_cast<Region_t>(dist(rng));
	});
	return map;
}