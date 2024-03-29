#include <DisRegRep/Filter/ExplicitRegionAreaFilter.hpp>
#include <DisRegRep/Filter/FilterTrait.hpp>

#include <DisRegRep/Container/BlendHistogram.hpp>
#include <DisRegRep/Container/HistogramCache.hpp>
#include <DisRegRep/Maths/Arithmetic.hpp>

#include <ranges>
#include <utility>

using std::any, std::any_cast;
using std::shared_ptr;
using std::views::iota;

using namespace DisRegRep;
namespace BH = BlendHistogram;
namespace HC = HistogramCache;
using Format::SSize_t, Format::SizeVec2, Format::Region_t, Format::Radius_t;

namespace {

template<typename TNormHist, typename TCache>
struct ExRAHistogram {

	TNormHist Histogram;
	TCache Cache;

	void resize(const SizeVec2& histogram_size, const Region_t rc, Radius_t) {
		this->Histogram.resize(histogram_size, rc);
		this->Cache.resize(rc);
	}

	void clear() {
		this->Histogram.clear();
		this->Cache.clear();
	}

};
using ExRAdcdh = ExRAHistogram<BH::DenseNorm, HC::Dense>;
using ExRAdcsh = ExRAHistogram<BH::SparseNormSorted, HC::Dense>;
using ExRAscsh = ExRAHistogram<BH::SparseNormUnsorted, HC::Sparse>;

template<typename THist>
inline const auto& runFilter(const auto& desc, any& memory) {
	const auto& [map_ptr, offset, extent, radius] = desc;
	const RegionMap& map = *map_ptr;

	using Arithmetic::toSigned;
	const auto [off_x, off_y] = toSigned(offset);
	const auto [ext_x, ext_y] = toSigned(extent);
	const auto sradius = toSigned(radius);

	THist& exra_histogram = *any_cast<shared_ptr<THist>&>(memory);
	exra_histogram.clear();

	auto& [histogram, cache] = exra_histogram;

	const auto copy_cache_to_histogram = [&histogram, &cache = std::as_const(cache),
		kernel_area = 1.0 * Arithmetic::kernelArea(radius)](const auto x, const auto y) -> void {
		histogram(cache, x, y, kernel_area);
	};
	for (const auto y : iota(SSize_t { 0 }, ext_y)) {
		for (const auto x : iota(SSize_t { 0 }, ext_x)) {
			cache.clear();

			for (const auto ry : iota(off_y - sradius, off_y + sradius + 1)) {
				for (const auto rx : iota(off_x - sradius, off_x + sradius + 1)) {
					const Region_t region = map(x + rx, y + ry);
					cache.increment(region);
				}
			}

			//normalise bin and copy to output
			copy_cache_to_histogram(x, y);
		}
	}
	return histogram;
}

}

DEFINE_ALL_REGION_MAP_FILTER_ALLOC_FUNC(ExplicitRegionAreaFilter, ::ExRA)
DEFINE_ALL_REGION_MAP_FILTER_FILTER_FUNC_SCSH_DEF(ExplicitRegionAreaFilter, ::ExRA)