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

namespace DefaultSetting {

constexpr F::SizeVec2 Extent = { 128u, 128u };
constexpr F::Region_t RegionCount = 8u;
constexpr F::Radius_t Radius = 8u;

constexpr uint64_t Seed = 0x1CD4C39A662BF9CAull;
constexpr size_t CentroidCount = 10u;

void exportSetting(const fs::path& export_filename) {
	auto f = ofstream(export_filename);

	using Lnc::Utility::formatArray;
	println(f, "Extent = {}", formatArray(Extent));
	println(f, "Region-Count = {}", RegionCount);
	println(f, "Radius = {}", Radius);
	println(f, "Seed = 0x{:X}", Seed);
	print(f, "Centroid-Count = {}", CentroidCount);
}

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
		<typename RunTag>(const RunTag run_tag, auto& histogram_mem, auto& histogram) -> void {
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
	constexpr static F::Radius_t RadiusSweep = 15u,
		MinRadius = 2u,
		MaxRadius = 64u,
		RadiusSweepStress = 30u,
		MinRadiusStress = 8u,
		MaxRadiusStress = 256u;
	const F::Region_t RegionCountStress = 15u;
	constexpr static F::SizeVec2 ExtentStress = { 512u, 512u };

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
	run(regular_desc, ::generateSweepVariable<F::Radius_t, MinRadius, MaxRadius, RadiusSweep>(),
		"Compare", ::DefaultSetting::RegionCount, ::DefaultSetting::Extent);
	run(stress_desc, ::generateSweepVariable<F::Radius_t, MinRadiusStress, MaxRadiusStress, RadiusSweepStress>(),
		"Stress", RegionCountStress, ExtentStress);
} catch (const exception& e) {
	::handleException(e);
}

void runRegionCount(const ReportDescription& report_desc, const ::RunDescription& run_desc) try {
	constexpr static F::Region_t RegionSweep = 15u,
		MinRegion = 2u,
		MaxRegion = 30u;

	const auto& [report_root] = report_desc;
	const auto& [factory_arr, filter_arr] = run_desc;

	const auto sweep_region_count = ::generateSweepVariable<F::Region_t, MinRegion, MaxRegion, RegionSweep>();
	auto region_count_runner = Lnc::FilterRunner(report_root / "region-count");

	for (const auto factory : factory_arr) {
		region_count_runner.setFactory(*factory);
		for (const auto filter : filter_arr) {
			region_count_runner.Filter = filter;
			region_count_runner.sweepRegionCount(::DefaultSetting::Extent,
				::DefaultSetting::Radius, sweep_region_count);
		}
	}
} catch (const exception& e) {
	::handleException(e);
}

void run() {
	println("Initialising test environment...");
	//factory
	const auto random_factory = RandomRegionFactory(::DefaultSetting::Seed);
	const auto voronoi_factory = VoronoiRegionFactory(::DefaultSetting::Seed, ::DefaultSetting::CentroidCount);
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

		::DefaultSetting::exportSetting(report_root / "default-setting.txt");
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