#include <DisRegRep/Filter/SingleHistogramFilter.hpp>
#include <DisRegRep/Maths/Arithmetic.hpp>

#include <memory>

#include <algorithm>
#include <ranges>
#include <functional>

using std::any, std::any_cast;
using std::shared_ptr, std::make_shared;
using std::plus, std::minus;
using std::views::iota, std::views::take, std::views::drop,
	std::ranges::fill, std::ranges::copy;

using namespace DisRegRep;
using namespace Format;

namespace {

struct SHFHistogram {

	struct {

		DenseSingleHistogram Horizontal;
		DenseNormSingleHistogram Final;

	} Histogram;
	DenseSingleHistogram Cache;

};
using SHFHistogram_t = shared_ptr<SHFHistogram>;

}

any SingleHistogramFilter::allocateHistogram(const RegionMapFilter::LaunchDescription& desc) const {
	const RegionMap& map = *desc.Map;
	const auto [ext_x, ext_y] = desc.Extent;
	const size_t region_count = map.RegionCount;

	const size_t row_length = ext_x * region_count,
		histogram_size = row_length * ext_y,
		//horizontal pass requires padding above and below
		histogram_h_size = row_length * (ext_y + 2u * desc.Radius);
	return make_shared<::SHFHistogram>(::SHFHistogram {
		.Histogram = {
			.Horizontal = histogram_h_size,
			.Final = histogram_size
		},
		.Cache = region_count
	});
}

const DenseNormSingleHistogram& SingleHistogramFilter::filter(const RegionMapFilter::LaunchDescription& desc, any& memory) const {
	const auto& [map, offset, extent, radius] = desc;

	const auto [off_x, off_y] = Arithmetic::toSigned(offset);
	const auto [ext_x, ext_y] = Arithmetic::toSigned(extent);
	const auto sradius = Arithmetic::toSigned(radius);

	auto& [histogram, cache] = *any_cast<::SHFHistogram_t&>(memory);
	auto& [histogram_h, histogram_full] = histogram;
	
	const auto& [region_map, dim, region_count] = *map;
	const auto [dim_x, dim_y] = dim;
	const auto map_indexer = DefaultRegionMapIndexer(dim_x, dim_y);
	const auto hist_h_indexer = DefaultDenseHistogramIndexer(ext_x, ext_y + 2 * sradius, region_count),
		hist_full_indexer = DefaultDenseHistogramIndexer(ext_x, ext_y, region_count);

	const auto empty_cache = [&cache]() constexpr -> void {
		fill(cache, DenseBin_t { });
	};
	const auto copy_to_histogram_h = [&cache, &hist_h_indexer, &histogram_h](const auto x, const auto y) constexpr -> void {
		copy(cache, histogram_h.begin() + hist_h_indexer(x, y, 0));
	};
	/********************
	 * Horizontal pass
	 *******************/
	for (const auto y : iota(Arithmetic::ssize_t { 0 }, ext_y + 2 * sradius)) {
		//region map y value
		const auto rg_y = off_y + y - sradius;
		
		empty_cache();
		//compute initialise bin for the current row
		for (const auto rx : iota(off_x - sradius, off_x + sradius)) {
			const Region_t region = region_map[map_indexer(rx, rg_y)];
			cache[region]++;
		}
		copy_to_histogram_h(0, y);

		//sliding kernel
		for (const auto x : iota(Arithmetic::ssize_t { 1 }, ext_x)) {
			const auto rg_x = off_x + x;

			const Region_t removing_region = region_map[map_indexer(rg_x - sradius - 1, rg_y)],
				adding_region = region_map[map_indexer(rg_x + sradius, rg_y)];
			cache[removing_region]--;
			cache[adding_region]++;

			copy_to_histogram_h(x, y);
		}
	}

#pragma warning(push)
#pragma warning(disable: 4244)//type conversion
	const auto cache_op_histogram_h = [&cache, &histogram_h, &hist_h_indexer, region_count]
		<typename Op>(const auto x, const auto y, const Op& op) constexpr -> void {
		//basically add everything from existing histogram into the cache
		const auto bin = histogram_h | drop(hist_h_indexer(x, y, 0)) | take(region_count);
		Arithmetic::addRange(cache, op, bin, cache.begin());
	};
	const auto copy_to_output = [&cache, &histogram_full, &hist_full_indexer](const auto x, const auto y) -> void {
		Arithmetic::normalise(cache, histogram_full.begin() + hist_full_indexer(x, y, 0));
	};
#pragma warning(pop)
	const auto op_plus = plus { };
	const auto op_minus = minus { };
	/********************
	 * Vertical pass
	 ******************/
	for (const auto x : iota(Arithmetic::ssize_t { 0 }, ext_x)) {
		//build initial accumulator
		empty_cache();
		for (const auto ry : iota(Arithmetic::ssize_t { 0 }, 2 * sradius)) {
			//this operator will first convert uint16_t to int, and compiler gives a warning
			//this is fine, because int is representable
			cache_op_histogram_h(x, ry, op_plus);
		}
		copy_to_output(x, 0);

		//sliding kernel
		for (const auto y : iota(Arithmetic::ssize_t { 1 }, ext_y)) {
			cache_op_histogram_h(x, y - 1, op_minus);
			cache_op_histogram_h(x, y + 2 * sradius, op_plus);

			copy_to_output(x, y);
		}
	}
	
	return histogram_full;
}