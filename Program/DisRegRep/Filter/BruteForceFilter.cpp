#include <DisRegRep/Filter/BruteForceFilter.hpp>
#include <DisRegRep/Maths/Arithmetic.hpp>

#include <memory>

#include <algorithm>
#include <ranges>

using std::any, std::any_cast;
using std::shared_ptr, std::make_shared;
using std::ranges::fill,
	std::views::iota;

using namespace DisRegRep;
using namespace Format;

namespace {

struct BFHistogram {

	DenseNormSingleHistogram Histogram;
	DenseSingleHistogram Cache;

};
using BFHistogram_t = shared_ptr<BFHistogram>;

}

any BruteForceFilter::allocateHistogram(const LaunchDescription& desc) const {
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

const DenseNormSingleHistogram& BruteForceFilter::filter(const LaunchDescription& desc, any& memory) const {
	const auto& [map, offset, extent, radius] = desc;
	
	const auto [off_x, off_y] = Arithmetic::toSigned(offset);
	const auto [ext_x, ext_y] = Arithmetic::toSigned(extent);
	const auto sradius = Arithmetic::toSigned(radius);
	const double ext_area = 1.0 * ext_x * ext_y;

	auto& [histogram, cache] = *any_cast<::BFHistogram_t&>(memory);
	
	const auto& [region_map, dim, region_count] = *map;
	const auto [dim_x, dim_y] = dim;
	const auto map_indexer = DefaultRegionMapIndexer(dim_x, dim_y);
	const auto hist_indexer = DefaultDenseHistogramIndexer(ext_x, ext_y, region_count);

	for (const auto y : iota(Arithmetic::ssize_t { 0 }, ext_y)) {
		for (const auto x : iota(Arithmetic::ssize_t { 0 }, ext_x)) {
			//clear bin cache for every pixel
			fill(cache, DenseBin_t { });

			for (const auto ry : iota(off_y - sradius, off_y + sradius + 1)) {
				for (const auto rx : iota(off_x - sradius, off_x + sradius + 1)) {
					const Region_t region = region_map[map_indexer(x + rx, y + ry)];
					cache[region]++;
				}
			}

			//normalise bin and copy to output
#pragma warning(push)
#pragma warning(disable: 4244)//type conversion
			Arithmetic::scaleRange(cache, histogram.begin() + hist_indexer(x, y, 0), ext_area);
#pragma warning(pop)
		}
	}
	return histogram;
}