#include <DisRegRep/Factory/RegionMapFactory.hpp>
#include <DisRegRep/Factory/RandomRegionFactory.hpp>

#include <DisRegRep/Filter/RegionMapFilter.hpp>
#include <DisRegRep/Filter/BruteForceFilter.hpp>
#include <DisRegRep/Filter/SingleHistogramFilter.hpp>

#include "FilterRunner.hpp"
#include "Utility.hpp"

#include <span>
#include <array>
#include <algorithm>
#include <ranges>

#include <print>
#include <thread>

using std::span, std::array;
using std::ranges::copy,
	std::views::iota, std::views::stride, std::views::take;

using std::print, std::println;
using std::jthread;

using namespace DisRegRep;
namespace F = Format;
namespace Lnc = Launch;

namespace {

namespace DefaultSetting {

constexpr F::SizeVec2 Extent = { 256u, 256u };
constexpr size_t RegionCount = 15u;
constexpr F::Radius_t Radius = 32u;

}

void runRadius(const RegionMapFactory& map_factory, const span<const RegionMapFilter* const> filter_arr) {
	constexpr static F::Radius_t RadiusCount = 15u,
		MaxRadius = 256u;

	array<F::Radius_t, RadiusCount> sweep_radius { };
	copy(iota(F::Radius_t { 0 }) | stride(RadiusCount / MaxRadius) | take(RadiusCount), sweep_radius.begin());
	auto radius_runner = Lnc::FilterRunner("bench-radius");

	//we create a region map that is large enough to hold the biggest filter kernel
	F::RegionMap region_map = map_factory.allocateRegionMap(
		Lnc::Utility::calcMinimumDimension(::DefaultSetting::Extent, sweep_radius.back()));
	map_factory({
		.RegionCount = ::DefaultSetting::RegionCount
	}, region_map);

	for (const auto filter : filter_arr) {
		radius_runner.Filter = filter;
		radius_runner.sweepRadius(region_map, ::DefaultSetting::Extent, sweep_radius);
	}
}

void run() {
	print("Benchmark complete, cleaning up...");
}

}

int main() {
	println("Welcome to Discrete Region Representation filter benchmark system\n");
	::run();
	print("Program exit normally :)");
	return 0;
}