#include <DisRegRep/Filter/BruteForceFilter.hpp>
#include <DisRegRep/Maths/Arithmetic.hpp>

#include <DisRegRep/Container/HistogramCache.hpp>

#include <memory>
#include <utility>

#include <ranges>

using std::any, std::any_cast;
using std::shared_ptr, std::make_shared_for_overwrite;
using std::views::iota;

using namespace DisRegRep;
namespace SH = SingleHistogram;
namespace HC = HistogramCache;
using Format::SizeVec2, Format::Bin_t, Format::NormBin_t, Format::Region_t;

namespace {

template<typename TNormHist>
struct BFFHistogram {

	using NormHistogramType = TNormHist;

	NormHistogramType Histogram;
	HC::Dense Cache;

	void resize(const SizeVec2& histogram_size, const Region_t rc) {
		this->Histogram.resize(histogram_size, rc);
		this->Cache.resize(rc);
	}

	void clear() {
		this->Histogram.clear();
		this->Cache.clear();
	}

};
using BFFDenseHistogram = BFFHistogram<SH::DenseNorm>;
using BFFSparseHistogram = BFFHistogram<SH::SparseNorm>;

template<typename THist>
inline void makeAllocation(const auto& desc, any& output) {
	using THist_t = shared_ptr<THist>;

	THist* allocation;
	//compatibility check
	if (const auto* const bff = any_cast<THist_t>(&output);
		bff) {
		allocation = &**bff;
	} else {
		allocation = &*output.emplace<THist_t>(make_shared_for_overwrite<THist>());
	}
	allocation->resize(desc.Extent, desc.Map->RegionCount);
}

template<typename THist>
inline const auto& runFilter(const auto& desc, any& memory) {
	using THist_t = shared_ptr<THist>;
	constexpr static bool IsOutputDense = SH::DenseInstance<typename THist::NormHistogramType>;

	const auto& [map_ptr, offset, extent, radius] = desc;
	const RegionMap& map = *map_ptr;

	using Arithmetic::toSigned;
	const auto [off_x, off_y] = toSigned(offset);
	const auto [ext_x, ext_y] = toSigned(extent);
	const auto sradius = toSigned(radius);

	THist& bff_histogram = *any_cast<THist_t&>(memory);
	bff_histogram.clear();

	auto& [histogram, cache] = bff_histogram;

	const auto copy_cache_to_histogram = [&histogram, &cache = std::as_const(cache),
		kernel_area = 1.0 * Arithmetic::kernelArea(radius)](const auto x, const auto y) -> void {
		histogram(cache, x, y, kernel_area);
	};
	for (const auto y : iota(Arithmetic::ssize_t { 0 }, ext_y)) {
		for (const auto x : iota(Arithmetic::ssize_t { 0 }, ext_x)) {
			cache.clear();

			for (const auto ry : iota(off_y - sradius, off_y + sradius + 1)) {
				for (const auto rx : iota(off_x - sradius, off_x + sradius + 1)) {
					const Region_t region = map(x + rx, y + ry);
					cache.increment(region);
				}
			}

			//normalise bin and copy to output
#pragma warning(push)
#pragma warning(disable: 4244)//type conversion
			copy_cache_to_histogram(x, y);
#pragma warning(pop)
		}
	}
	return histogram;
}

}

void BruteForceFilter::tryAllocateHistogram(LaunchTag::Dense, const LaunchDescription& desc, any& output) const {
	::makeAllocation<::BFFDenseHistogram>(desc, output);
}

void BruteForceFilter::tryAllocateHistogram(LaunchTag::Sparse, const LaunchDescription& desc, any& output) const {
	::makeAllocation<::BFFSparseHistogram>(desc, output);
}

const SH::DenseNorm& BruteForceFilter::operator()(LaunchTag::Dense,
	const LaunchDescription& desc, any& memory) const {
	return ::runFilter<::BFFDenseHistogram>(desc, memory);
}

const SH::SparseNorm& BruteForceFilter::operator()(LaunchTag::Sparse,
	const LaunchDescription& desc, any& memory) const {
	return ::runFilter<::BFFSparseHistogram>(desc, memory);
}