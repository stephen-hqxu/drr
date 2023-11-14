#include "BruteForceFilter.hpp"

#include <memory>

#include <execution>
#include <algorithm>
#include <ranges>
#include <numeric>

using std::any, std::any_cast;
using std::shared_ptr, std::make_shared;
using std::ranges::transform, std::ranges::fill,
	std::views::iota,
	std::reduce;

using namespace DisRegRep;
using namespace Format;

namespace {

struct BFHistogram {

	DenseNormSingleHistogram Histogram;
	DenseSingleHistogram Cache;

};
using BFHistogram_t = shared_ptr<BFHistogram>;

}

any BruteForceFilter::allocateHistogram(const RegionMapFilter::LaunchDescription& desc) const {
	const RegionMap& map = *desc.Map;
	const auto [ext_x, ext_y] = desc.Extent;
	const size_t region_count = map.RegionCount;

	const size_t histogram_size = ext_x * ext_y * region_count;

	//shame on you `std::any` for only taking copyable type!!!
	return make_shared<::BFHistogram>(::BFHistogram {
		.Histogram = histogram_size,
		.Cache = region_count
	});
}

const DenseNormSingleHistogram& BruteForceFilter::filter(const RegionMapFilter::LaunchDescription& desc, any& memory) const {
	const auto& [map, offset, extent, radius] = desc;
	
	const auto [ext_x, ext_y] = Indexer::toSigned(extent);
	const auto sradius = Indexer::toSigned(radius);

	auto& [histogram, cache] = *any_cast<::BFHistogram_t&>(memory);
	
	const auto indexer = RegionMapFilter::Indexer(desc);
	const auto& region_map = map->Map;
	for (const auto y : iota(Indexer::SignedSize_t { 0 }, ext_y)) {
		for (const auto x : iota(Indexer::SignedSize_t { 0 }, ext_x)) {
			//clear bin cache for every pixel
			fill(cache, DenseBin_t { });

			for (const auto ry : iota(-sradius, sradius)) {
				for (const auto rx : iota(-sradius, sradius)) {
					const RegionMap_t region = region_map[indexer.region(x + rx, y + ry)];
					cache[region]++;
				}
			}

			//normalise bin and copy to output
#pragma warning(push)
#pragma warning(disable: 4244)//type conversion
			const double sum = reduce(std::execution::unseq, cache.cbegin(), cache.cend());
			transform(cache, histogram.begin() + indexer.histogram(x, y, 0),
				[sum](const auto count) constexpr noexcept { return static_cast<float>(static_cast<double>(count) / sum); });
#pragma warning(pop)
		}
	}
	return histogram;
}