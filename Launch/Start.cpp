#include <DisRegRep/Factory/RegionMapFactory.hpp>
#include <DisRegRep/Factory/RandomRegionFactory.hpp>
#include <DisRegRep/Factory/VoronoiRegionFactory.hpp>

#include <DisRegRep/Filter/RegionMapFilter.hpp>
#include <DisRegRep/Filter/BruteForceFilter.hpp>
#include <DisRegRep/Filter/SingleHistogramFilter.hpp>

#include "FilterRunner.hpp"

#include <span>
#include <array>
#include <execution>
#include <algorithm>
#include <ranges>

#include <iostream>
#include <print>

#include <thread>
#include <exception>
#include <cstdint>

using std::span, std::array, std::any;
using std::ranges::copy,
	std::views::iota, std::views::stride, std::views::take, std::views::drop,
	std::equal;

using std::print, std::println;
using std::jthread;

using namespace DisRegRep;
namespace F = Format;
namespace Lnc = Launch;

using RMFT = DisRegRep::RegionMapFilter::LaunchTag;
using RunDense = RMFT::Dense;
using RunSparse = RMFT::Sparse;

namespace {

namespace DefaultSetting {

constexpr F::SizeVec2 Extent = { 128u, 128u };
constexpr size_t RegionCount = 8u;
constexpr F::Radius_t Radius = 8u;

constexpr uint64_t Seed = 0x1CD4C39A662BF9CAull;
constexpr size_t Centroid = 30u;

}

using FactoryCollection = span<const RegionMapFactory* const>;
using FilterCollection = span<const RegionMapFilter* const>;

struct TestDescription {

	const RegionMapFactory& RegionMapFactory;

	const struct {

		const RegionMapFilter& GroundTruth;
		const FilterCollection Verifying;

	} Filter;

};

struct RunDescription {

	const FactoryCollection Factory;
	const FilterCollection Filter;

};

//The main purpose is to make sure our optimised implementation is sound
bool selfTest(const TestDescription& desc) {
	const auto& [map_factory, filter] = desc;
	const auto& [ground_truth, verifying] = filter;

	const F::RegionMap region_map = map_factory({
		.RegionCount = 15u
	}, { 12u, 12u });
	const RegionMapFilter::LaunchDescription launch_desc {
		.Map = &region_map,
		.Offset = { 2u, 2u },
		.Extent = { 8u, 8u },
		.Radius = 2u
	};

	any hist_gt_mem;
	ground_truth.tryAllocateHistogram(launch_desc, hist_gt_mem);
	const F::DenseNormSingleHistogram& hist_gt = ground_truth(RunDense { }, launch_desc, hist_gt_mem);
	for (const auto cmp_filter : verifying) {
		any hist_cmp_mem;
		cmp_filter->tryAllocateHistogram(launch_desc, hist_cmp_mem);
		const F::DenseNormSingleHistogram& hist_cmp = (*cmp_filter)(RunDense { }, launch_desc, hist_cmp_mem);

		if (!std::equal(std::execution::unseq, hist_cmp.cbegin(), hist_cmp.cend(), hist_gt.cbegin())) {
			return false;
		}
	}
	return true;
}

template<typename T, T Min, T Max, T Count>
auto generateSweepVariable() {
	array<T, Count> sweep { };
	copy(iota(Min) | stride(Max / Count) | take(Count), sweep.begin());
	return sweep;
}

void runRadius(const ::RunDescription& regular_desc, const ::RunDescription& stress_desc) {
	constexpr static F::Radius_t RadiusSweep = 15u,
		MinRadius = 2u,
		MaxRadius = 64u,
		RadiusSweepStress = 30u,
		MinRadiusStress = 8u,
		MaxRadiusStress = 256u;
	const size_t RegionCountStress = 15u;
	constexpr static F::SizeVec2 ExtentStress = { 512u, 512u };

	auto run = [radius_runner = Lnc::FilterRunner("bench-radius")](const ::RunDescription& desc, const auto sweep_radius,
		const char* const tag, const size_t region_count, const F::SizeVec2& extent) mutable -> void {
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
}

void runRegionCount(const ::RunDescription& desc) {
	constexpr static size_t RegionSweep = 15u,
		MinRegion = 2u,
		MaxRegion = 30u;

	const auto& [factory_arr, filter_arr] = desc;

	const auto sweep_region_count = ::generateSweepVariable<size_t, MinRegion, MaxRegion, RegionSweep>();
	auto region_count_runner = Lnc::FilterRunner("bench-region_count");

	for (const auto factory : factory_arr) {
		region_count_runner.setFactory(*factory);
		for (const auto filter : filter_arr) {
			region_count_runner.Filter = filter;
			region_count_runner.sweepRegionCount(::DefaultSetting::Extent,
				::DefaultSetting::Radius, sweep_region_count);
		}
	}
}

void run() {
	println("Initialising test environment...");
	//factory
	const auto random_factory = RandomRegionFactory(::DefaultSetting::Seed);
	const auto voronoi_factory = VoronoiRegionFactory(::DefaultSetting::Seed, ::DefaultSetting::Centroid);
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
	const bool test_result = ::selfTest({
		.RegionMapFactory = voronoi_factory,
		.Filter = {
			.GroundTruth = bf,
			.Verifying = filter | drop(1u)
		}
	});
	println("Self test complete, returning test status: {}\n", test_result ? "PASS" : "FAIL");
	if (!test_result) {
		return;
	}

	println("Running benchmark, please wait...");
	{
		const ::RunDescription desc {
			.Factory = factory,
			.Filter = filter
		};

		using std::cref;
		const array<jthread, 2u> worker {
			jthread(::runRadius, cref(desc), ::RunDescription {
				.Factory = span(factory).subspan(0u, 1u),
				.Filter = span(filter).subspan(1u, 1u)
			}),
			jthread(::runRegionCount, cref(desc))
		};
	}
	print("Benchmark complete, cleaning up...");
}

}

int main() {
	println("Welcome to Discrete Region Representation filter benchmark system\n");
	try {
		::run();
	} catch (const std::exception& e) {
		println(std::cerr, "Error occurred during execution: {}", e.what());
		return 1;
	}
	println("Program exit normally :)");
	return 0;
}