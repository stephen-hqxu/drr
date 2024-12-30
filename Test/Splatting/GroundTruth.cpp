#include <DisRegRep/Launch/FilterTester.hpp>

#include <DisRegRep/Format.hpp>
#include <DisRegRep/Maths/Arithmetic.hpp>
#include <DisRegRep/Container/RegionMap.hpp>
#include <DisRegRep/Container/BlendHistogram.hpp>

#include "Utility.hpp"

#include <any>
#include <array>
#include <string_view>
#include <tuple>

#include <algorithm>
#include <functional>
#include <ranges>

#include <type_traits>
#include <utility>

#include <cstdint>

using std::array, std::tuple,
	std::string_view, std::any;
using std::ranges::copy, std::ranges::all_of, std::ranges::adjacent_find,
	std::views::enumerate;

using std::get, std::apply,
	std::make_index_sequence, std::index_sequence;
using std::is_same_v;

using namespace DisRegRep::Format;
namespace BH = DisRegRep::BlendHistogram;
using DisRegRep::RegionMap, DisRegRep::RegionMapFactory, DisRegRep::RegionMapFilter;
using DisRegRep::Arithmetic::kernelArea;
using namespace DisRegRep::Launch;

namespace {

constexpr array<Region_t, 64u> TestRegionMapTemplate = {
	0, 2, 1, 3, 1, 1, 1, 2,
	0, 1, 1, 2, 2, 2, 2, 0,
	3, 3, 0, 3, 2, 2, 1, 0,
	0, 1, 2, 0, 2, 2, 0, 2,
	3, 0, 3, 2, 1, 2, 2, 3,
	0, 3, 0, 2, 1, 0, 3, 2,
	0, 1, 0, 2, 0, 0, 0, 0,
	3, 3, 0, 3, 3, 3, 1, 0
};
constexpr struct {

	SizeVec2 Dimension = { 8u, 8u },
		Offset = { 2u, 2u },
		Extent = { 4u, 4u };
	Radius_t Radius = 2u;
	Region_t RegionCount = 4u;

} TestRegionMapInfo { };

RegionMap createTestRegionMap() {
	RegionMap rm;
	rm.reshape(::TestRegionMapInfo.Dimension);
	rm.RegionCount = ::TestRegionMapInfo.RegionCount;

	copy(::TestRegionMapTemplate, rm.begin());
	return rm;
}

BH::DenseNorm createTestSingleHistogram() {
	BH::DenseNorm hist;
	const auto write = [&hist, norm_factor = 1.0 * kernelArea(::TestRegionMapInfo.Radius)]
	(const array<Bin_t, ::TestRegionMapInfo.RegionCount> bin, const uint8_t x, const uint8_t y) -> void {
		hist(bin, x, y, norm_factor);
	};
	hist.resize(::TestRegionMapInfo.Extent, ::TestRegionMapInfo.RegionCount);

	//bin 0-3
	write({ 6, 6, 7, 6 }, 0, 0);
	write({ 3, 7, 11, 4 }, 1, 0);
	write({ 3, 7, 12, 3 }, 2, 0);
	write({ 4, 5, 13, 3 }, 3, 0);
	//bin 4-7
	write({ 7, 5, 7, 6 }, 0, 1);
	write({ 5, 5, 11, 4 }, 1, 1);
	write({ 5, 4, 13, 3 }, 2, 1);
	write({ 5, 3, 14, 3 }, 3, 1);
	//bin 8-11
	write({ 9, 4, 6, 6 }, 0, 2);
	write({ 8, 4, 9, 4 }, 1, 2);
	write({ 9, 3, 10, 3 }, 2, 2);
	write({ 8, 3, 11, 3 }, 3, 2);
	//bin 12-15
	write({ 9, 4, 5, 7 }, 0, 3);
	write({ 8, 4, 7, 6 }, 1, 3);
	write({ 9, 3, 8, 5 }, 2, 3);
	write({ 8, 3, 9, 5 }, 3, 3);

	return hist;
}

constexpr RegionMapFilter::LaunchDescription createLaunchDescription(const RegionMap& region_map) noexcept {
	return {
		.Map = &region_map,
		.Offset = ::TestRegionMapInfo.Offset,
		.Extent = ::TestRegionMapInfo.Extent,
		.Radius = ::TestRegionMapInfo.Radius
	};
}

}

template<size_t TestSize>
bool FilterTester::test(const TestDescription<TestSize>& desc) {
	const auto [factory, filter] = desc;
	const RegionMap region_map = ::createTestRegionMap();
	const BH::DenseNorm hist_ground_truth = ::createTestSingleHistogram();
	const RegionMapFilter::LaunchDescription launch_desc = ::createLaunchDescription(region_map);

	/**********************
	 * Generate histogram
	 *********************/
	array<array<any, TestSize>, Utility::AllFilterTagSize> hist_mem;
	array<BH::SparseNormSorted, TestSize> sorted_hist_mem;
	tuple<
		array<const BH::DenseNorm*, TestSize>,
		array<const BH::SparseNormSorted*, TestSize>,
		array<const BH::SparseNormSorted*, TestSize>
	> hist;
	
	const auto generate_histogram = [&filter, &launch_desc, &sorted_hist_mem]
		<typename Tag>(const Tag run_tag, auto& hist_mem, auto& hist) -> void {
		for (const auto [i, curr_filter_ptr] : enumerate(filter)) {
			const RegionMapFilter& curr_filter = *curr_filter_ptr;

			any& curr_hist_mem = hist_mem[i];
			curr_filter.tryAllocateHistogram(run_tag, launch_desc, curr_hist_mem);
			if constexpr (is_same_v<Tag, Utility::RMFT::SCSH>) {
				sorted_hist_mem[i] = std::move(const_cast<BH::SparseNormUnsorted&>(curr_filter(run_tag, launch_desc, curr_hist_mem)));
				hist[i] = &sorted_hist_mem[i];
			} else {
				hist[i] = &curr_filter(run_tag, launch_desc, curr_hist_mem);
			}
		}
	};
	const auto generate_all_histogram = [&generate_histogram, &hist_mem, &hist]
		<size_t... I>(index_sequence<I...>) -> void {
		(generate_histogram(get<I>(Utility::AllFilterTag), hist_mem[I], get<I>(hist)), ...);
	};
	generate_all_histogram(make_index_sequence<Utility::AllFilterTagSize> { });

	/*************************
	 * Ground truth test
	 ************************/
	constexpr static auto deref = [](const auto* const ptr) constexpr static noexcept -> const auto& {
		return *ptr;
	};

	const bool gt_test_status = apply([&hist_ground_truth](const auto&... hist) {
		return (all_of(hist, [&hist_ground_truth](const auto& hist) { return hist == hist_ground_truth; }, deref) && ...);
	}, hist);
	const bool same_type_cmp_test_status = apply([](const auto&... hist) {
		return ((adjacent_find(hist, std::not_equal_to { }, deref) == hist.cend()) && ...);
	}, hist);
	const bool cross_type_cmp_test_status = apply([](const auto& first_hist, const auto&... other_hist) {
		return ((deref(first_hist.front()) == deref(other_hist.front())) && ...);
	}, hist);

	return gt_test_status && same_type_cmp_test_status && cross_type_cmp_test_status;
}

template bool FilterTester::test<2u>(const FilterTester::TestDescription<2u>&);