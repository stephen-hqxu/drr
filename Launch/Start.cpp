#include <DisRegRep/Container/SingleHistogram.hpp>

#include <DisRegRep/Factory/RegionMapFactory.hpp>
#include <DisRegRep/Factory/RandomRegionFactory.hpp>
#include <DisRegRep/Factory/VoronoiRegionFactory.hpp>

#include <DisRegRep/Filter/RegionMapFilter.hpp>
#include <DisRegRep/Filter/BruteForceFilter.hpp>
#include <DisRegRep/Filter/SingleHistogramFilter.hpp>

#include "FilterRunner.hpp"
#include "FilterTester.hpp"
#include "Utility.hpp"

#include <string>
#include <span>
#include <array>

#include <execution>
#include <algorithm>
#include <ranges>

#include <iostream>
#include <fstream>
#include <filesystem>
#include <print>
#include <format>

#include <exception>
#include <cstdint>

using std::span, std::array, std::string;
using std::ranges::copy, std::ranges::max_element,
	std::views::iota, std::views::stride, std::views::take,
	std::views::zip;

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

		using value_type = T;

		value_type Sweep, Min, Max;

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
	SweepContext<size_t> SweepCentroid;

} DefaultSetting = {
//We keep two different sets of settings for debug and release mode.
//In debug mode we are mainly trying to hunt for bugs, so turn down the settings to speed things up.
#ifndef NDEBUG
	.Extent = { 16, 16 },
	.RegionCount = 4,
	.Radius = 2,
	.CentroidCount = 4,

	.SweepRadius = { 4, 2, 10 },
	.SweepRadiusStress = {
		.SweepRadius = { 8, 4, 32 },
		.Extent = { 64, 64 },
		.RegionCount = 8
	},
	.SweepRegion = { 6, 2, 8 },
	.SweepCentroid = { 4, 1, 10 }
#else//NDEBUG
	.Extent = { 192, 192 },
	.RegionCount = 5,
	.Radius = 16,
	.CentroidCount = 5,

	.SweepRadius = { 15, 2, 64 },
	.SweepRadiusStress = {
		.SweepRadius = { 30, 8, 256 },
		.Extent = { 512, 512 },
		.RegionCount = 15
	},
	.SweepRegion = { 15, 2, 30 },
	.SweepCentroid = { 15, 1, 60 }
#endif//NDEBUG
};
//An alias for default setting
constexpr auto& DS = ::DefaultSetting;

template<typename T, const ::BenchmarkContext::SweepContext<T>& Ctx>
consteval auto generateSweepVariable() {
	array<T, Ctx.Sweep> sweep { };
	copy(iota(Ctx.Min) | stride(Ctx.Max / Ctx.Sweep) | take(Ctx.Sweep), sweep.begin());
	return sweep;
}

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
	{
		BRING_OUT_SETTING(SweepCentroid);
		println(f, "Sweep Centroid");
		EXPORT_SETTING;
	}
#undef EXPORT_SETTING
#undef BRING_OUT_SETTING
}

template<size_t FacSize, size_t FiltSize>
struct RunDescription {

	Lnc::FilterRunner& Runner;
	const span<const RegionMapFactory* const, FacSize> Factory;
	const span<const RegionMapFilter* const, FiltSize> Filter;

};

constexpr auto RadiusVariation = ::generateSweepVariable<F::Radius_t, ::DS.SweepRadius>();
constexpr auto RadiusStressVariation = ::generateSweepVariable<F::Radius_t, ::DS.SweepRadiusStress.SweepRadius>();
constexpr auto RegionVariation = ::generateSweepVariable<F::Region_t, ::DS.SweepRegion>();
constexpr auto CentroidVariation = ::generateSweepVariable<size_t, ::DS.SweepCentroid>();

void handleException(const exception& e) {
	println(std::cerr, "Error occurs during execution: {}", e.what());
}

template<size_t S1, size_t S2, size_t S3, size_t S4>
void runRadius(const ::RunDescription<S1, S2>& regular_desc, const span<RegionMap, S1> regular_rm,
	const ::RunDescription<S3, S4>& stress_desc, const span<RegionMap, S3> stress_rm) {
	const auto run = [](const auto& desc, const auto& region_map_arr, const F::Region_t region_count,
		const F::SizeVec2& extent, const auto& sweep_radius, const string& tag = { }) -> void {
			const auto& [runner, factory_arr, filter_arr] = desc;
			for (const auto [factory, region_map] : zip(factory_arr, region_map_arr)) {
				Lnc::Utility::generateMinimumRegionMap(region_map, *factory, extent,
					*max_element(sweep_radius), region_count);

				runner.sweepRadius({
					.Factory = factory,
					.Filter = filter_arr,
					.UserTag = tag
				}, region_map, extent, sweep_radius);
			}
		};
	{
		run(regular_desc, regular_rm, ::DS.RegionCount, ::DS.Extent, ::RadiusVariation);
	}
	{
		constexpr auto& SRS = ::DS.SweepRadiusStress;
		run(stress_desc, stress_rm, SRS.RegionCount, SRS.Extent, ::RadiusStressVariation, "Stress");
	}
}

template<size_t S1, size_t S2>
void runRegionCount(const ::RunDescription<S1, S2>& run_desc) {
	const auto& [runner, factory_arr, filter_arr] = run_desc;

	for (const auto factory : factory_arr) {
		runner.sweepRegionCount({
			.Factory = factory,
			.Filter = filter_arr
		}, ::DS.Extent, ::DS.Radius, ::RegionVariation);
	}
}

template<size_t S1, size_t S2>
void runCentroidCount(const ::RunDescription<S1, S2>& run_desc) {
	const auto& [runner, _, filter_arr] = run_desc;

	runner.sweepCentroidCount({
		.Filter = filter_arr
	}, ::DS.Extent, ::DS.Radius, ::DS.RegionCount, ::DS.Seed, ::CentroidVariation);
}

void run() {
	println("Initialising test environment...");
	//factory
	const auto random_factory = RandomRegionFactory(::BenchmarkContext::Seed);
	const auto voronoi_factory = VoronoiRegionFactory(::BenchmarkContext::Seed, ::DS.CentroidCount);
	//filter
	const BruteForceFilter bf;
	const SingleHistogramFilter shf;

	const array<const RegionMapFactory*, 2u> factory_array {
		&random_factory, &voronoi_factory
	};
	const array<const RegionMapFilter*, 2u> filter_array {
		&bf, &shf
	};
	const span factory = factory_array;
	const span filter = filter_array;

	array<RegionMap, filter_array.size()> rm_radius_regular_array;
	array<RegionMap, 1u> rm_radius_stress_array;
	const span rm_radius_regular = rm_radius_regular_array;
	const span rm_radius_stress = rm_radius_stress_array;
	println("Initialisation complete\n");

	println("Performing self test, this should only take a fractional of a second...");
	const bool test_result = Lnc::FilterTester::test(Lnc::FilterTester::TestDescription {
		.Factory = &random_factory,
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
		::runRadius(run_desc, rm_radius_regular, ::RunDescription {
			.Runner = runner,
			.Factory = factory.subspan<0u, 1u>(),
			.Filter = filter.subspan<1u, 1u>()
		}, rm_radius_stress);
		::runRegionCount(::RunDescription {
			.Runner = runner,
			.Factory = factory.subspan<1u, 1u>(),
			.Filter = filter
		});
		::runCentroidCount(run_desc);

		::exportSetting(report_root / "default-setting.txt");

		runner.waitAll();
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