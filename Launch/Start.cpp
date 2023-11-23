#include <DisRegRep/Container/SingleHistogram.hpp>

#include <DisRegRep/Factory/RegionMapFactory.hpp>
#include <DisRegRep/Factory/RandomRegionFactory.hpp>
#include <DisRegRep/Factory/VoronoiRegionFactory.hpp>

#include <DisRegRep/Filter/RegionMapFilter.hpp>
#include <DisRegRep/Filter/BruteForceFilter.hpp>
#include <DisRegRep/Filter/SingleHistogramFilter.hpp>

#include "FilterRunner.hpp"
#include "Utility.hpp"

#include <string_view>
#include <span>
#include <array>
#include <tuple>

#include <execution>
#include <algorithm>
#include <ranges>
#include <functional>

#include <iostream>
#include <fstream>
#include <filesystem>
#include <print>
#include <format>

#include <exception>
#include <type_traits>
#include <cstdint>

using std::span, std::array, std::string_view, std::any;
using std::ranges::copy,
	std::views::iota, std::views::stride, std::views::take, std::views::drop, std::views::enumerate,
	std::equal;

namespace fs = std::filesystem;
using std::print, std::println, std::format, std::ofstream;
using std::exception;

using namespace DisRegRep;
namespace SH = SingleHistogram;
namespace F = Format;
namespace Lnc = Launch;

namespace {

//We store all benchmark settings here!
constexpr struct BenchmarkContext {

	constexpr static uint64_t Seed = 0x1CD4C39A662BF9CAull;

	//Common parameters for sweeping a variable.
	template<typename T>
	struct SweepContext {

		T Sweep, Min, Max;

	};

	F::SizeVec2 Extent;
	F::Region_t RegionCount;
	F::Radius_t Radius;
	size_t CentroidCount;

	SweepContext<F::Radius_t> SweepRadius;
	struct {

		SweepContext<F::Radius_t> SweepRadius;
		F::SizeVec2 Extent;
		F::Region_t RegionCount;

	} SweepRadiusStress;
	SweepContext<F::Region_t> SweepRegion;

} DefaultSetting = {
//We keep two different sets of settings for debug and release mode.
//In debug mode we are mainly trying to hunt for bugs, so turn down the settings to speed things up.
#ifndef NDEBUG
	.Extent = { 16u, 16u },
	.RegionCount = 4u,
	.Radius = 2u,
	.CentroidCount = 4u,

	.SweepRadius = { 4u, 2u, 10u },
	.SweepRadiusStress = {
		.SweepRadius = { 8u, 4u, 32u },
		.Extent = { 64u, 64u },
		.RegionCount = 8u
	},
	.SweepRegion = { 6u, 2u, 8u }
#else//NDEBUG
	.Extent = { 192u, 192u },
	.RegionCount = 5u,
	.Radius = 16u,
	.CentroidCount = 5u,

	.SweepRadius = { 15u, 2u, 64u },
	.SweepRadiusStress = {
		.SweepRadius = { 30u, 8u, 256u },
		.Extent = { 512u, 512u },
		.RegionCount = 15u
	},
	.SweepRegion = { 15u, 2u, 30u }
#endif//NDEBUG
};
//An alias for default setting
constexpr auto& DS = ::DefaultSetting;

void exportSetting(const fs::path& export_filename) {
	auto f = ofstream(export_filename);

	using Lnc::Utility::formatArray;
	println(f, "Default Setting:");
	println(f, "Extent = {}", formatArray(::DS.Extent));
	println(f, "Region-Count = {}", ::DS.RegionCount);
	println(f, "Radius = {}", ::DS.Radius);
	println(f, "Seed = 0x{:X}", ::DS.Seed);
	println(f, "Centroid-Count = {}\n", ::DS.CentroidCount);

	const auto export_sweep_setting = [&f](const auto sweep, const auto min, const auto max) -> void {
		println(f, "Sweep = {}", sweep);
		println(f, "Min = {}", min);
		println(f, "Max = {}\n", max);
		};
#define BRING_OUT_SETTING(SET_NAME) constexpr auto& S = ::DS.SET_NAME
#define EXPORT_SETTING export_sweep_setting(S.Sweep, S.Min, S.Max)
	{
		BRING_OUT_SETTING(SweepRadius);
		println(f, "Sweep Radius:");
		EXPORT_SETTING;
	}
	{
		{
			BRING_OUT_SETTING(SweepRadiusStress);
			println(f, "Sweep Radius Stress:");
			println(f, "Extent = {}", formatArray(S.Extent));
			println(f, "Region-Count = {}", S.RegionCount);
		}
		{
			BRING_OUT_SETTING(SweepRadiusStress.SweepRadius);
			EXPORT_SETTING;
		}
	}
	{
		BRING_OUT_SETTING(SweepRegion);
		println(f, "Sweep Region:");
		EXPORT_SETTING;
	}
#undef EXPORT_SETTING
#undef BRING_OUT_SETTING
}

using FactoryCollection = span<const RegionMapFactory* const>;
using FilterCollection = span<const RegionMapFilter* const>;

template<size_t TestSize>
struct TestDescription {

	const RegionMapFactory& Factory;
	const span<const RegionMapFilter* const, TestSize> Filter;

};

struct RunDescription {

	Lnc::FilterRunner& Runner;
	const FactoryCollection Factory;
	const FilterCollection Filter;

};

//The main purpose is to make sure our optimised implementation is sound
template<size_t TestSize>
requires(TestSize > 0u)
bool selfTest(const TestDescription<TestSize>& desc) {
	const auto [factory, filter] = desc;
	const RegionMap region_map = factory({
		.RegionCount = 15u
	}, { 12u, 12u });
	const RegionMapFilter::LaunchDescription launch_desc {
		.Map = &region_map,
		.Offset = { 2u, 2u },
		.Extent = { 8u, 8u },
		.Radius = 2u
	};

	const auto run_test = [&desc, &launch_desc, &filter]
		(const auto run_tag, auto& histogram_mem, auto& histogram) -> void {
		for (const auto [i, curr_filter_ptr] : enumerate(filter)) {
			const RegionMapFilter& curr_filter = *curr_filter_ptr;

			any& curr_hist_mem = histogram_mem[i];
			curr_filter.tryAllocateHistogram(run_tag, launch_desc, curr_hist_mem);
			histogram[i] = &curr_filter(run_tag, launch_desc, curr_hist_mem);
		}
	};
	
	using std::get, std::apply, std::ranges::transform;
	array<array<any, TestSize>, Lnc::Utility::AllFilterTagSize> hist_mem;
	std::tuple<
		array<const SH::DenseNorm*, TestSize>,
		array<const SH::SparseNormSorted*, TestSize>,
		array<const SH::SparseNormUnsorted*, TestSize>
	> hist;
	array<SH::SparseNormSorted, TestSize> sorted_sparse;
	array<const SH::SparseNormSorted*, TestSize> sorted_sparse_ptr;
	
	const auto run_all_test = [&run_test, &hist_mem, &hist]<size_t... I>(std::index_sequence<I...>) -> void {
		(run_test(get<I>(Lnc::Utility::AllFilterTag), hist_mem[I], get<I>(hist)), ...);
	};
	run_all_test(std::make_index_sequence<Lnc::Utility::AllFilterTagSize> { });

	//we cannot compare unsorted histogram directly, need to sort them first
	transform(get<2u>(hist), sorted_sparse.begin(),
		[]<typename T>(const T* const unsorted) constexpr noexcept -> T&& {
			return std::move(*const_cast<T*>(unsorted));
		});
	transform(sorted_sparse, sorted_sparse_ptr.begin(), [](const auto& hist) constexpr noexcept { return &hist; });

	/*********************
	 * Correctness check
	 ********************/
	using std::ranges::adjacent_find, std::is_same_v;
	const auto all_hist_equal = [not_eq_op = std::not_equal_to { }](const auto& hist) -> bool {
		return adjacent_find(hist, not_eq_op,
			[](const auto* const ptr) constexpr noexcept -> const auto& { return *ptr; }) == hist.cend();
	};
	const auto same_type_compare = [&sorted_sparse_ptr, &all_hist_equal]
		<typename T>(const array<const T*, TestSize>& hist) -> bool {
		if constexpr (is_same_v<T, SH::SparseNormUnsorted>) {
			return all_hist_equal(sorted_sparse_ptr);
		} else {
			return all_hist_equal(hist);
		}
	};
	const auto cross_type_get_candidate = [&sorted_sparse_ptr, &hist]
		<typename T>(const array<const T*, TestSize>& hist) constexpr noexcept -> const auto& {
		if constexpr (is_same_v<T, SH::SparseNormUnsorted>) {
			return *sorted_sparse_ptr.front();
		} else {
			return *hist.front();
		}
	};

	return apply([&same_type_compare](const auto&... h) {
		return (same_type_compare(h) && ...);
	}, hist)
	&& apply([&cross_type_get_candidate](const auto& h, const auto&... hs) {
		return ((cross_type_get_candidate(h) == cross_type_get_candidate(hs)) && ...);
	}, hist);
}

template<typename T, T Min, T Max, T Count>
auto generateSweepVariable() {
	array<T, Count> sweep { };
	copy(iota(Min) | stride(Max / Count) | take(Count), sweep.begin());
	return sweep;
}

void handleException(const exception& e) {
	println(std::cerr, "Error occurs during execution: {}", e.what());
}

void runRadius(const ::RunDescription& regular_desc, const ::RunDescription& stress_desc) {
	auto run = [](const ::RunDescription& desc, const auto sweep_radius,
		const string_view tag, const F::Region_t region_count, const F::SizeVec2& extent) mutable -> void {
			const auto& [runner, factory_arr, filter_arr] = desc;
			for (const auto factory : factory_arr) {
				runner.sweepRadius({
					.Factory = factory,
					.Filter = filter_arr,
					.UserTag = tag
				}, extent, sweep_radius, region_count);
			}
		};
	{
		constexpr auto& SR = ::DS.SweepRadius;
		run(regular_desc, ::generateSweepVariable<F::Radius_t, SR.Min, SR.Max, SR.Sweep>(),
			"Default", ::DS.RegionCount, ::DS.Extent);
	}
	{
		constexpr auto& SRS = ::DS.SweepRadiusStress;
		constexpr auto& SR = SRS.SweepRadius;
		run(stress_desc, ::generateSweepVariable<F::Radius_t, SR.Min, SR.Max, SR.Sweep>(),
			"Stress", SRS.RegionCount, SRS.Extent);
	}
}

void runRegionCount(const ::RunDescription& run_desc) {
	const auto& [runner, factory_arr, filter_arr] = run_desc;

	constexpr auto& SR = ::DS.SweepRegion;
	const auto sweep_region_count = ::generateSweepVariable<F::Region_t, SR.Min, SR.Max, SR.Sweep>();

	for (const auto factory : factory_arr) {
		runner.sweepRegionCount({
			.Factory = factory,
			.Filter = filter_arr
		}, ::DS.Extent, ::DS.Radius, sweep_region_count);
	}
}

void run() {
	println("Initialising test environment...");
	//factory
	const auto random_factory = RandomRegionFactory(::BenchmarkContext::Seed);
	const auto voronoi_factory = VoronoiRegionFactory(::BenchmarkContext::Seed, ::DS.CentroidCount);
	//filter
	const BruteForceFilter bf;
	const SingleHistogramFilter shf;

	const array<const RegionMapFactory*, 2u> factory {
		&random_factory, &voronoi_factory
	};
	const array<const RegionMapFilter*, 2u> filter {
		&bf, &shf
	};
	println("Initialisation complete\n");

	println("Performing self test, this should only take a fractional of a second...");
	const bool test_result = ::selfTest<filter.size()>({
		.Factory = random_factory,
		.Filter = filter
		});
	println("Self test complete, returning test status: {}\n", test_result ? "PASS" : "FAIL");
	if (!test_result) {
		return;
	}

	println("Running benchmark, please wait...");
	{
		const auto ts = Lnc::Utility::TimestampClock::now();
		const auto report_root = fs::path(format("bench-result_{}", Lnc::Utility::formatTimestamp(ts)));
		fs::create_directory(report_root);

		auto runner = Lnc::FilterRunner(report_root);

		const ::RunDescription run_desc {
			.Runner = runner,
			.Factory = factory,
			.Filter = filter
		};
		::runRadius(run_desc, {
			.Runner = runner,
			.Factory = span(factory).subspan(0u, 1u),
			.Filter = span(filter).subspan(1u, 1u)
		});
		::runRegionCount(run_desc);

		::exportSetting(report_root / "default-setting.txt");
	}
	print("Benchmark complete, cleaning up...");
}

}

int main() {
	println("Welcome to Discrete Region Representation filter benchmark system\n");
	try {
		::run();
	} catch (const exception& e) {
		::handleException(e);
		return 1;
	}
	println("Program exit normally :)");
	return 0;
}