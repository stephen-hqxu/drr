#include "SingleHistogramFilter.hpp"

#include <memory>

#include <algorithm>
#include <ranges>
#include <numeric>
#include <execution>
#include <functional>

using std::any, std::any_cast;
using std::shared_ptr, std::make_shared;
using std::plus, std::minus;
using std::views::iota, std::views::take, std::views::drop,
	std::ranges::fill, std::ranges::transform, std::ranges::copy,
	std::reduce,
	std::execution::unseq;

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

	const auto [ext_x, ext_y] = Indexer::toSigned(extent);
	const auto sradius = Indexer::toSigned(radius);

	auto& [histogram, cache] = *any_cast<::SHFHistogram_t&>(memory);
	auto& [histogram_h, histogram_full] = histogram;
	
	const auto indexer = RegionMapFilter::Indexer(desc);
	const auto& region_map = map->Map;
	const size_t region_count = map->RegionCount;

	const auto empty_cache = [&cache]() constexpr -> void {
		fill(cache, DenseBin_t { });
	};
	const auto copy_to_histogram_h = [&cache, &indexer, &histogram_h](const auto x, const auto y) constexpr -> void {
		copy(cache, histogram_h.begin() + indexer.histogram(x, y, 0));
	};
	/********************
	 * Horizontal pass
	 *******************/
	for (const auto y : iota(Indexer::SignedSize_t { 0 }, ext_y + 2 * sradius)) {
		//region map y value
		const auto rg_y = y - sradius;
		
		empty_cache();
		//compute initialise bin for the current row
		for (const auto rx : iota(-sradius, sradius)) {
			const RegionMap_t region = region_map[indexer.region(rx, rg_y)];
			cache[region]++;
		}
		copy_to_histogram_h(0, y);

		//sliding kernel
		for (const auto x : iota(Indexer::SignedSize_t { 1 }, ext_x)) {
			const RegionMap_t removing_region = region_map[indexer.region(x - sradius - 1, rg_y)],
				adding_region = region_map[indexer.region(x + sradius, rg_y)];
			cache[removing_region]--;
			cache[adding_region]++;

			copy_to_histogram_h(x, y);
		}
	}

	//WORKAROUND: A bug on MSVC causes internal compiler error on nested template in lambda.
	//Have to pass the operator in initialised.
	const auto cache_op_histogram_h = [&cache, &histogram_h, &indexer, region_count]
		<typename Op>(const auto x, const auto y, Op&& op) constexpr -> void {
		//basically add everything from existing histogram into the cache
		const auto bin = histogram_h | drop(indexer.histogram(x, y, 0)) | take(region_count);
		std::transform(unseq, cache.cbegin(), cache.cend(), bin.cbegin(), cache.begin(), std::forward<Op>(op));
	};
	//CONSIDER: refactor this function (along with the one in brute force kernel) to share implementation.
#pragma warning(push)
#pragma warning(disable: 4244)//type conversion
	const auto copy_to_output = [&cache, &histogram_full, &indexer](const auto x, const auto y) -> void {
		const double sum = reduce(unseq, cache.cbegin(), cache.cend());
		transform(cache, histogram_full.begin() + indexer.histogram(x, y, 0),
			[sum](const auto count) constexpr noexcept { return static_cast<float>(static_cast<double>(count) / sum); });
	};
#pragma warning(pop)
	/********************
	 * Vertical pass
	 ******************/
	for (const auto x : iota(Indexer::SignedSize_t { 0 }, ext_x)) {
		//build initial accumulator
		empty_cache();
		for (const auto ry : iota(0, 2 * sradius)) {
			cache_op_histogram_h(x, ry, plus<DenseBin_t> { });
		}
		copy_to_output(x, 0);

		//sliding kernel
		for (const auto y : iota(Indexer::SignedSize_t { 1 }, ext_y)) {
			cache_op_histogram_h(x, y - 1, minus<DenseBin_t> { });
			cache_op_histogram_h(x, y + 2 * sradius, plus<DenseBin_t> { });

			copy_to_output(x, y);
		}
	}
	
	return histogram_full;
}