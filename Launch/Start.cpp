#include <DisRegRep/Factory/RegionMapFactory.hpp>
#include <DisRegRep/Factory/RandomRegionFactory.hpp>

#include <DisRegRep/Filter/RegionMapFilter.hpp>
#include <DisRegRep/Filter/BruteForceFilter.hpp>
#include <DisRegRep/Filter/SingleHistogramFilter.hpp>

#include <nb/nanobench.h>

#include <array>
#include <algorithm>
#include <ranges>

#include <memory>
#include <any>

#include <print>
#include <chrono>
#include <ratio>

#include <cstdint>

using namespace DisRegRep;
namespace nb = ankerl::nanobench;
using namespace std;
using chrono::steady_clock, chrono::duration, chrono::duration_cast, std::milli;

namespace {

constexpr uint32_t Seed = 0b00111101100100101010000100100010u;
constexpr array<size_t, 2u> Extent = { 1024u, 1024u };
constexpr size_t RegionCount = 15u,
	KernelRadius = 128u;

constexpr auto Offset = []() consteval {
	auto offset = Extent;
	offset.fill(KernelRadius);
	return offset;
}();
constexpr auto MapDimension = []() consteval {
	auto dim = Extent;
	ranges::transform(dim, dim.begin(), [](const auto i) constexpr { return i + KernelRadius * 2u; });
	return dim;
}();

using Timer = steady_clock;
using Timestamp = Timer::time_point;

constexpr uint32_t calcElapsed(const Timestamp start, const Timestamp stop) {
	return duration_cast<duration<uint32_t, milli>>(stop - start).count();
}

Format::RegionMap createRegionMap(const RegionMapFactory& factory) {
	return factory({
		.Dimension = ::MapDimension,
		.RegionCount = ::RegionCount
	});
}

}

int main() {
	print("Discrete Region Representation benchmark program :)\n");
	print("Please note that time measured in this program are all in ms\n\n");

	/*********************
	 * Create region map
	 *********************/
	print("Region map initialisation\n");
	const auto factory = RandomRegionFactory(::Seed);
	auto start = Timer::now();
	const Format::RegionMap region_map = ::createRegionMap(factory);
	auto end = Timer::now();
	print("Region map initialisation successful, time elapsed {}\n", ::calcElapsed(start, end));

	/**********************
	 * Allocate histogram
	 *********************/
	print("Allocate single histogram\n");
	const SingleHistogramFilter filter;
	const RegionMapFilter::LaunchDescription filter_desc {
		.Map = &region_map,
		.Offset = ::Offset,
		.Extent = ::Extent,
		.Radius = ::KernelRadius
	};
	any histogram = filter.allocateHistogram(filter_desc);
	print("Single histogram allocation done\n");

	/********************
	 * Filter
	 *******************/
	print("Running single histogram filter\n");
	print("Now running: brute-force kernel... ");
	start = Timer::now();
	filter.filter(filter_desc, histogram);
	end = Timer::now();
	print("Done, time elapsed: {}\n", ::calcElapsed(start, end));

	print("\nFinish workflow successfully, cleaning up and exit...");
	return 0;
}