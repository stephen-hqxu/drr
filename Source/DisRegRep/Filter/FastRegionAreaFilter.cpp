#include <DisRegRep/Filter/FastRegionAreaFilter.hpp>
#include <DisRegRep/Filter/FilterTrait.hpp>

#include <DisRegRep/Container/BlendHistogram.hpp>
#include <DisRegRep/Container/HistogramCache.hpp>
#include <DisRegRep/Maths/Arithmetic.hpp>

#include <ranges>
#include <functional>

#include <utility>
#include <type_traits>

using std::any, std::any_cast;
using std::shared_ptr;
using std::views::iota;

using namespace DisRegRep;
namespace BH = BlendHistogram;
namespace HC = HistogramCache;
using Format::SSize_t, Format::SizeVec2, Format::Region_t, Format::Radius_t;

namespace {

//Calculate dimension for horizontal pass histogram.
constexpr SizeVec2 calcHorizontalHistogramDimension(SizeVec2 histogram_size, const Radius_t radius) noexcept {
	//Remember we have transposed x,y axes during filtering to improve cache locality.
	//It does improve performance by around 10%, which is pretty decent :)
	//Therefore the horizontal pass requires padding above and below.
	histogram_size[0] += 2u * radius;
	return histogram_size;
}

template<typename THist, typename TNormHist, typename TCache>
struct ReABSAHistogram {

	struct {

		THist Horizontal;
		TNormHist Final;

	} Histogram;
	TCache Cache;

	void resize(const SizeVec2& histogram_size, const Region_t rc, const Radius_t radius) {
		auto& [hori, final] = this->Histogram;
		hori.resize(::calcHorizontalHistogramDimension(histogram_size, radius), rc);
		final.resize(histogram_size, rc);
		this->Cache.resize(rc);
	}

	void clear() {
		auto& [hori, final] = this->Histogram;
		hori.clear();
		final.clear();
		this->Cache.clear();
	}

};
using ReABSAdcdh = ReABSAHistogram<BH::Dense, BH::DenseNorm, HC::Dense>;
using ReABSAdcsh = ReABSAHistogram<BH::SparseSorted, BH::SparseNormSorted, HC::Dense>;
using ReABSAscsh = ReABSAHistogram<BH::SparseUnsorted, BH::SparseNormUnsorted, HC::Sparse>;

template<typename THist>
inline const auto& runFilter(const auto& desc, any& memory) {
	const auto& [map_ptr, offset, extent, radius] = desc;
	const RegionMap& map = *map_ptr;

	using std::as_const, Arithmetic::toSigned;
	const auto [off_x, off_y] = toSigned(offset);
	const auto [ext_x, ext_y] = toSigned(extent);
	const auto sradius = toSigned(radius);
	const auto sradius_2 = 2 * sradius;

	THist& fast_histogram = *any_cast<shared_ptr<THist>&>(memory);
	fast_histogram.clear();

	auto& [histogram, cache] = fast_histogram;
	auto& [histogram_h, histogram_full] = histogram;

	const auto copy_to_histogram_h = [&cache = as_const(cache), &histogram_h](const auto x, const auto y) -> void {
		//remember axes are swapped
		histogram_h(cache, y, x);
	};
	/********************
	 * Horizontal pass
	 *******************/
	for (const auto y : iota(SSize_t { 0 }, ext_y + sradius_2)) {
		//region map y value
		const auto rg_y = off_y + y - sradius;
		
		cache.clear();
		//compute initialise bin for the current row
		for (const auto rx : iota(off_x - sradius, off_x + sradius + 1)) {
			const Region_t region = map[rx, rg_y];
			cache.increment(region);
		}
		copy_to_histogram_h(0, y);

		//sliding kernel
		for (const auto x : iota(SSize_t { 1 }, ext_x)) {
			const auto rg_x = off_x + x;

			const Region_t removing_region = map[rg_x - sradius - 1, rg_y],
				adding_region = map[rg_x + sradius, rg_y];
			cache.decrement(removing_region);
			cache.increment(adding_region);

			copy_to_histogram_h(x, y);
		}
	}

	using std::plus, std::minus, std::is_same_v;
	//For some reason the `as_const` here on horizontal histogram is very important,
	//	benchmark shows up-to 16% of improvement in timing for using const v.s. non-const version.
	const auto cache_op_histogram_h = [&cache, &histogram_h = as_const(histogram_h)]
		<typename Op>(const auto x, const auto y, Op) -> void {
		//basically add everything from existing histogram into the cache
		//also remember that axes are swapped
		const auto bin_view = histogram_h[y, x];
		if constexpr (is_same_v<Op, plus<void>>) {
			cache.increment(bin_view);
		} else {
			cache.decrement(bin_view);
		}
	};
	const auto copy_to_output = [&cache = as_const(cache), &histogram_full,
		kernel_area = 1.0 * Arithmetic::kernelArea(radius)](const auto x, const auto y) -> void {
		histogram_full(cache, x, y, kernel_area);
	};
	const auto op_plus = plus { };
	const auto op_minus = minus { };
	/********************
	 * Vertical pass
	 ******************/
	for (const auto x : iota(SSize_t { 0 }, ext_x)) {
		//build initial accumulator
		cache.clear();
		for (const auto ry : iota(SSize_t { 0 }, sradius_2 + 1)) {
			//this operator will first convert uint16_t to int, and compiler gives a warning
			//this is fine, because int is representable
			cache_op_histogram_h(x, ry, op_plus);
		}
		copy_to_output(x, 0);

		//sliding kernel
		for (const auto y : iota(SSize_t { 1 }, ext_y)) {
			cache_op_histogram_h(x, y - 1, op_minus);
			cache_op_histogram_h(x, y + sradius_2, op_plus);

			copy_to_output(x, y);
		}
	}
	
	return histogram_full;
}

}

DEFINE_ALL_REGION_MAP_FILTER_ALLOC_FUNC(FastRegionAreaFilter, ::ReABSA)
DEFINE_ALL_REGION_MAP_FILTER_FILTER_FUNC_DEF(FastRegionAreaFilter, ::ReABSA)