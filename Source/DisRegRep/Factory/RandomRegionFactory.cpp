#include <DisRegRep/Factory/RandomRegionFactory.hpp>

#include <nb/nanobench.h>

#include <algorithm>

using ankerl::nanobench::Rng;

using std::ranges::generate;

using namespace DisRegRep;
using namespace Format;

void RandomRegionFactory::operator()(const CreateDescription& desc, RegionMap& output) const {
	const auto [region_count] = desc;
	output.RegionCount = region_count;

	auto rng = Rng(this->RandomSeed);
	generate(output, [&rng, rc = static_cast<uint32_t>(region_count)]() noexcept {
		return static_cast<Region_t>(rng.bounded(rc));
	});
}