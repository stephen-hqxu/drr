#include <DisRegRep/Factory/RandomRegionFactory.hpp>

#include <random>
#include <algorithm>

using std::ranlux24, std::uniform_int_distribution;
using std::ranges::generate;

using namespace DisRegRep;
using namespace Format;

RegionMap RegionMapFactory::allocateRegionMap(const SizeVec2& dimension) const {
	const auto [x, y] = dimension;
	return {
		.Map = x * y,
		.Dimension = dimension,
	};
}

RegionMap RegionMapFactory::operator()(const RegionMapFactory::CreateDescription& desc, const SizeVec2& dimension) const {
	RegionMap map = this->allocateRegionMap(dimension);
	(*this)(desc, map);
	return map;
}

void RandomRegionFactory::operator()(const RegionMapFactory::CreateDescription& desc, RegionMap& output) const {
	const auto [region_count] = desc;
	output.RegionCount = region_count;

	generate(output.Map, [rng = ranlux24(static_cast<uint_fast32_t>(this->RandomSeed)),
		dist = uniform_int_distribution<uint64_t>(0u, region_count - 1u)]() mutable {
		return static_cast<Region_t>(dist(rng));
	});
}