#include <DisRegRep/Factory/RegionMapFactory.hpp>
#include <DisRegRep/Factory/RandomRegionFactory.hpp>

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

namespace {

namespace DefaultSetting {

constexpr F::SizeVec2 Extent = { 128u, 128u };
constexpr size_t RegionCount = 8u;
constexpr F::Radius_t Radius = 16u;

constexpr uint32_t Seed = 0b11111001011000010100101110110111u;

}

using FilterCollection = span<const RegionMapFilter* const>;

struct TestDescription {

	const RegionMapFactory& RegionMapFactory;

	const struct {

		const RegionMapFilter& GroundTruth;
		const FilterCollection Verifying;

	} Filter;

};

struct RunDescription {

	const RegionMapFactory& RegionMapFactory;
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

	any hist_gt_mem = ground_truth.allocateHistogram(launch_desc);
	const F::DenseNormSingleHistogram& hist_gt = ground_truth.filter(launch_desc, hist_gt_mem);
	for (const auto cmp_filter : verifying) {
		any hist_cmp_mem = cmp_filter->allocateHistogram(launch_desc);
		const F::DenseNormSingleHistogram& hist_cmp = cmp_filter->filter(launch_desc, hist_cmp_mem);

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

void runRadius(const ::RunDescription& desc) {
	constexpr static F::Radius_t RadiusSweep = 15u,
		MaxRadius = 64u;

	const auto sweep_radius = ::generateSweepVariable<F::Radius_t, 2u, MaxRadius, RadiusSweep>();
	auto radius_runner = Lnc::FilterRunner("bench-radius");

	const auto& [map_factory, filter_arr] = desc;
	//we create a region map that is large enough to hold the biggest filter kernel
	const F::RegionMap region_map = map_factory({
		.RegionCount = ::DefaultSetting::RegionCount
	}, Lnc::Utility::calcMinimumDimension(::DefaultSetting::Extent, sweep_radius.back()));

	for (const auto filter : filter_arr) {
		radius_runner.Filter = filter;
		radius_runner.sweepRadius(region_map, ::DefaultSetting::Extent, sweep_radius);
	}
}

void runRegionCount(const ::RunDescription& desc) {
	constexpr static size_t RegionSweep = 15u,
		MaxRegion = 30u;

	const auto sweep_region_count = ::generateSweepVariable<size_t, 2u, MaxRegion, RegionSweep>();
	auto region_count_runner = Lnc::FilterRunner("bench-region_count");

	const auto& [map_factory, filter_arr] = desc;
	for (const auto filter : filter_arr) {
		region_count_runner.Filter = filter;
		region_count_runner.sweepRegionCount(map_factory, ::DefaultSetting::Extent,
			::DefaultSetting::Radius, sweep_region_count);
	}
}

void run() {
	println("Initialising test environment...");
	//factory
	RandomRegionFactory random_factory;
	random_factory.RandomSeed = ::DefaultSetting::Seed;
	//filter
	BruteForceFilter bf;
	SingleHistogramFilter shf;

	const array<const RegionMapFactory*, 1u> factory {
		&random_factory
	};
	const array<const RegionMapFilter*, 2u> filter {
		&bf, &shf
	};
	println("Initialisation complete\n");

	println("Performing self test, this should only take a fractional of a second...");
	const bool test_result = ::selfTest({
		.RegionMapFactory = random_factory,
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
		//CONSIDER: Add more region map factory for testing in the future.
		const ::RunDescription desc {
			.RegionMapFactory = *factory[0],
			.Filter = filter
		};

		using std::cref;
		const array<jthread, 2u> worker {
			jthread(::runRadius, cref(desc)),
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