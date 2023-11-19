#include <DisRegRep/Filter/SingleHistogramFilter.hpp>
#include <DisRegRep/Maths/Arithmetic.hpp>

#include <memory>

#include <span>
#include <algorithm>
#include <ranges>
#include <functional>

using std::any, std::any_cast;
using std::shared_ptr, std::make_shared_for_overwrite;
using std::plus, std::minus;
using std::span;
using std::views::iota,
	std::ranges::fill, std::ranges::copy;

using namespace DisRegRep;
namespace SH = SingleHistogram;
using Format::SizeVec2, Format::Radius_t, Format::Bin_t, Format::Region_t;

namespace {

constexpr SizeVec2 OneD = { 1u, 1u };

//Calculate dimension for horizontal pass histogram.
constexpr SizeVec2 calcHorizontalHistogramDimension(SizeVec2 histogram_size, const Radius_t radius) noexcept {
	//Remember we have transposed x,y axes during filtering to improve cache locality.
	//It does improve performance by around 10%, which is pretty decent :)
	//Therefore the horizontal pass requires padding above and below.
	histogram_size[0] += 2u * radius;
	return histogram_size;
}

template<typename THist, typename TNormHist>
struct SHFHistogram {

	struct {

		THist Horizontal;
		TNormHist Final;

	} Histogram;
	SH::Dense Cache;

	template<typename = void>
	requires(SH::DenseInstance<THist> && SH::DenseInstance<TNormHist>)
	constexpr void resize(const SizeVec2& histogram_size, const size_t rc, const Radius_t radius) {
		auto& [hori, final] = this->Histogram;
		hori.reshape(::calcHorizontalHistogramDimension(histogram_size, radius), rc);
		final.reshape(histogram_size, rc);
		this->Cache.reshape(::OneD, rc);
	}

	template<typename = void>
	requires(SH::SparseInstance<THist> && SH::SparseInstance<TNormHist>)
	constexpr void resize(const SizeVec2& histogram_size, const size_t rc, const Radius_t radius) {
		auto& [hori, final] = this->Histogram;
		hori.reshape(::calcHorizontalHistogramDimension(histogram_size, radius));
		final.reshape(histogram_size);
		this->Cache.reshape(::OneD, rc);
	}

};
using SHFDenseHistogram = SHFHistogram<SH::Dense, SH::DenseNorm>;
using SHFSparseHistogram = SHFHistogram<SH::Sparse, SH::SparseNorm>;

using SHFDenseHistogram_t = shared_ptr<SHFDenseHistogram>;
using SHFSparseHistogram_t = shared_ptr<SHFSparseHistogram>;

template<typename THist>
void makeAllocation(const auto& desc, any& output) {
	using THist_t = shared_ptr<THist>;

	THist* allocation;
	if (const auto* const shf = any_cast<THist_t>(&output);
		shf) {
		allocation = &**shf;
	} else {
		allocation = &*output.emplace<THist_t>(make_shared_for_overwrite<THist>());
	}
	allocation->resize(desc.Extent, desc.Map->RegionCount, desc.Radius);
}

}

void SingleHistogramFilter::tryAllocateHistogram(LaunchTag::Dense, const LaunchDescription& desc, any& output) const {
	::makeAllocation<::SHFDenseHistogram>(desc, output);
}

void SingleHistogramFilter::tryAllocateHistogram(LaunchTag::Sparse, const LaunchDescription& desc, any& output) const {
	const auto& ext = desc.Extent;
	const auto [ext_x, ext_y] = ext;

	::makeAllocation<::SHFSparseHistogram>(desc, output);
}

const SH::DenseNorm& SingleHistogramFilter::operator()(LaunchTag::Dense,
	const LaunchDescription& desc, any& memory) const {
	const auto& [map_ptr, offset, extent, radius] = desc;
	const RegionMap& map = *map_ptr;
	const size_t region_count = map.RegionCount;

	using Arithmetic::toSigned;
	const auto [off_x, off_y] = toSigned(offset);
	const auto [ext_x, ext_y] = toSigned(extent);
	const auto sradius = toSigned(radius);
	const auto sradius_2 = 2 * sradius;
	const double ext_area = 1.0 * Arithmetic::horizontalProduct(extent);

	auto& [histogram, cache_hist] = *any_cast<::SHFDenseHistogram_t&>(memory);
	const SH::Dense::BinView cache = cache_hist(0u, 0u, 0u);
	auto& [histogram_h, histogram_full] = histogram;

	const auto empty_cache = [&cache]() constexpr -> void {
		fill(cache, Bin_t { });
	};
	const auto copy_to_histogram_h = [&cache, &histogram_h](const auto x, const auto y) constexpr -> void {
		//axes swapped
		copy(cache, histogram_h(y, x, 0).begin());
	};
	/********************
	 * Horizontal pass
	 *******************/
	for (const auto y : iota(Arithmetic::ssize_t { 0 }, ext_y + sradius_2)) {
		//region map y value
		const auto rg_y = off_y + y - sradius;
		
		empty_cache();
		//compute initialise bin for the current row
		for (const auto rx : iota(off_x - sradius, off_x + sradius + 1)) {
			const Region_t region = map(rx, rg_y);
			cache[region]++;
		}
		copy_to_histogram_h(0, y);

		//sliding kernel
		for (const auto x : iota(Arithmetic::ssize_t { 1 }, ext_x)) {
			const auto rg_x = off_x + x;

			const Region_t removing_region = map(rg_x - sradius - 1, rg_y),
				adding_region = map(rg_x + sradius, rg_y);
			cache[removing_region]--;
			cache[adding_region]++;

			copy_to_histogram_h(x, y);
		}
	}

#pragma warning(push)
#pragma warning(disable: 4244)//type conversion
	const auto cache_op_histogram_h = [&cache, &histogram_h, region_count]
		(const auto x, const auto y, const auto& op) constexpr -> void {
		//basically add everything from existing histogram into the cache
		//also axes swapped
		const auto bin = span(histogram_h(y, x, 0).cbegin(), region_count);
		Arithmetic::addRange(cache, op, bin, cache.begin());
	};
	const auto copy_to_output = [&cache, &histogram_full, ext_area](const auto x, const auto y) -> void {
		Arithmetic::scaleRange(cache, histogram_full(x, y, 0).begin(), ext_area);
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
		for (const auto ry : iota(Arithmetic::ssize_t { 0 }, sradius_2 + 1)) {
			//this operator will first convert uint16_t to int, and compiler gives a warning
			//this is fine, because int is representable
			cache_op_histogram_h(x, ry, op_plus);
		}
		copy_to_output(x, 0);

		//sliding kernel
		for (const auto y : iota(Arithmetic::ssize_t { 1 }, ext_y)) {
			cache_op_histogram_h(x, y - 1, op_minus);
			cache_op_histogram_h(x, y + sradius_2, op_plus);

			copy_to_output(x, y);
		}
	}
	
	return histogram_full;
}

const SH::SparseNorm& SingleHistogramFilter::operator()(LaunchTag::Sparse,
	const LaunchDescription& desc, any& memory) const {
	auto& [histogram, cache] = *any_cast<::SHFSparseHistogram_t&>(memory);
	auto& [histogram_h, histogram_full] = histogram;

	return histogram_full;
}