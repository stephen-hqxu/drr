#include <DisRegRep/Container/SingleHistogram.hpp>

#include <DisRegRep/Factory/RegionMapFactory.hpp>
#include <DisRegRep/Factory/RandomRegionFactory.hpp>
#include <DisRegRep/Factory/VoronoiRegionFactory.hpp>

#include <DisRegRep/Filter/RegionMapFilter.hpp>
#include <DisRegRep/Filter/BruteForceFilter.hpp>
#include <DisRegRep/Filter/SingleHistogramFilter.hpp>

#include "FilterRunner.hpp"
#include "Utility.hpp"

#include <span>
#include <array>

#include <execution>
#include <algorithm>
#include <ranges>
#include <functional>

#include <iostream>
#include <fstream>
#include <filesystem>
#include <print>
#include <format>

#include <thread>
#include <exception>
#include <cstdint>

using std::span, std::array, std::any;
using std::ranges::copy,
	std::views::iota, std::views::stride, std::views::take, std::views::drop, std::views::enumerate,
	std::equal;

namespace fs = std::filesystem;
using std::print, std::println, std::format, std::ofstream;
using std::jthread, std::exception;

using namespace DisRegRep;
namespace SH = SingleHistogram;
namespace F = Format;
namespace Lnc = Launch;

namespace {

using RMFT = DisRegRep::RegionMapFilter::LaunchTag;
constexpr auto RunDense = RMFT::Dense { };
constexpr auto RunSparse = RMFT::Sparse { };

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
	.Extent = { 128u, 128u },
	.RegionCount = 8u,
	.Radius = 8u,
	.CentroidCount = 10u,

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

struct ReportDescription {

	const fs::path& ReportRoot;

};

struct RunDescription {

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
		<typename RunTag>(const RunTag run_tag, auto & histogram_mem, auto & histogram) -> void {
		for (const auto [i, curr_filter_ptr] : enumerate(filter)) {
			const RegionMapFilter& curr_filter = *curr_filter_ptr;

			any& curr_hist_mem = histogram_mem[i];
			curr_filter.tryAllocateHistogram(run_tag, launch_desc, curr_hist_mem);
			histogram[i] = &curr_filter(run_tag, launch_desc, curr_hist_mem);
		}
	};
	array<any, TestSize> dense_mem, sparse_mem;
	array<const SH::DenseNorm*, TestSize> dense_hist;
	array<const SH::SparseNorm*, TestSize> sparse_hist;
	run_test(::RunDense, dense_mem, dense_hist);
	run_test(::RunSparse, sparse_mem, sparse_hist);

	using std::ranges::adjacent_find;
	const auto fun_not_eq = std::not_equal_to { };
	constexpr static auto dereferencer = [](const auto* const ptr) constexpr noexcept -> const auto& {
		return *ptr;
		};
	return adjacent_find(dense_hist, fun_not_eq, dereferencer) == dense_hist.cend()
		&& adjacent_find(sparse_hist, fun_not_eq, dereferencer) == sparse_hist.cend()
		&& *dense_hist.front() == *sparse_hist.front();
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

void runRadius(const ReportDescription& report_desc, const ::RunDescription& regular_desc,
	const ::RunDescription& stress_desc) try {
	const auto& [report_root] = report_desc;

	auto radius_runner = Lnc::FilterRunner(report_root / "radius");
	auto run = [&radius_runner](const ::RunDescription& desc, const auto sweep_radius,
		const char* const tag, const F::Region_t region_count, const F::SizeVec2& extent) mutable -> void {
			const auto& [factory_arr, filter_arr] = desc;
			radius_runner.UserTag = tag;
			radius_runner.setRegionCount(region_count);

			for (const auto factory : factory_arr) {
				radius_runner.setFactory(*factory);
				for (const auto filter : filter_arr) {
					radius_runner.Filter = filter;
					radius_runner.sweepRadius(extent, sweep_radius);
				}
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
} catch (const exception& e) {
	::handleException(e);
}

void runRegionCount(const ReportDescription& report_desc, const ::RunDescription& run_desc) try {
	const auto& [report_root] = report_desc;
	const auto& [factory_arr, filter_arr] = run_desc;

	constexpr auto& SR = ::DS.SweepRegion;
	const auto sweep_region_count = ::generateSweepVariable<F::Region_t, SR.Min, SR.Max, SR.Sweep>();
	auto region_count_runner = Lnc::FilterRunner(report_root / "region-count");

	for (const auto factory : factory_arr) {
		region_count_runner.setFactory(*factory);
		for (const auto filter : filter_arr) {
			region_count_runner.Filter = filter;
			region_count_runner.sweepRegionCount(::DS.Extent,
				::DS.Radius, sweep_region_count);
		}
	}
} catch (const exception& e) {
	::handleException(e);
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

		const ::ReportDescription report_desc {
			.ReportRoot = report_root
		};
		const ::RunDescription run_desc {
			.Factory = factory,
			.Filter = filter
		};

		using std::cref;
		const array<jthread, 2u> worker {
			jthread(::runRadius, cref(report_desc), cref(run_desc), ::RunDescription {
				.Factory = span(factory).subspan(0u, 1u),
				.Filter = span(filter).subspan(1u, 1u)
				}),
			jthread(::runRegionCount, cref(report_desc), cref(run_desc))
		};

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