#include "FilterRunner.hpp"
#include "Utility.hpp"

#include <nb/nanobench.h>

#include <string>
#include <array>

#include <algorithm>

#include <format>
#include <fstream>
#include <chrono>
#include <stdexcept>

#include <charconv>
#include <concepts>
#include <limits>

using std::ranges::max_element;

using std::string, std::string_view, std::span, std::array;
using std::to_chars;
using std::ofstream, std::runtime_error, std::format;

namespace fs = std::filesystem;
namespace nb = ankerl::nanobench;
using namespace DisRegRep::Format;
using namespace DisRegRep::Launch;

namespace {

constexpr char RenderResultTemplate[] = R"DELIM(x,iter,t_median,t_mean,t_mdape,t_total
{{#result}}{{name}},{{sum(iterations)}},{{median(elapsed)}},{{average(elapsed)}},{{medianAbsolutePercentError(elapsed)}},{{sumProduct(elapsed,iterations)}}
{{/result}})DELIM";

string formatSize(const SizeVec2& size) {
	const auto [x, y] = size;
	return format("({}, {})", x, y);
}

template<std::integral T>
auto toString(const T input) {
	//one more space for null at the end
	array<char, std::numeric_limits<T>::digits10 + 1u> str { };

	char* const first = str.data();
	if (to_chars(first, first + str.size(), input).ec != std::errc { }) {
		throw runtime_error("Input conversion to string failed");
	}
	return str;
}

}

FilterRunner::FilterRunner(const string_view test_report_dir) :
	ReportRoot(test_report_dir), Factory(nullptr), RegionCount { }, DirtyMap(true), Filter(nullptr) {
	//canonical path will remove potential trailing slash
	this->ReportRoot = fs::weakly_canonical(this->ReportRoot);
	{
		using namespace std::chrono;
		const auto now = zoned_time(current_zone(), system_clock::now()).get_local_time();

		//add a timestamp to the output directory
		this->ReportRoot.replace_filename(format("{}-{:%Y-%m-%d_%H-%M}", this->ReportRoot.filename().string(), now));
	}
	if (!fs::create_directory(this->ReportRoot)) {
		throw runtime_error(format("Unable use directory \'{}\'.", this->ReportRoot.string()));
	}
}

auto FilterRunner::createBenchmark() {
	using namespace std::chrono_literals;

	nb::Bench bench;
	bench.timeUnit(1ms, "ms").unit("run")
		.epochs(15u).minEpochTime(5ms).maxEpochTime(10s)
		.performanceCounters(false)
		//need to disable console output because we are running with multiple threads.
		.output(nullptr);
	return bench;
}

void FilterRunner::renderReport(auto& bench) {
	const fs::path report_file = (this->ReportRoot / format("{0}({2},{1}).csv", bench.title(),
		this->Filter->name(), this->Factory->name()));

	auto csv = ofstream(report_file.string());
	bench.render(::RenderResultTemplate, csv);
}

inline void FilterRunner::refreshMap(const SizeVec2& new_extent, const Radius_t radius) {
	const SizeVec2 new_dimension = Utility::calcMinimumDimension(new_extent, radius);
	if (RegionMapFactory::reshape(this->Map, new_dimension)) {
		this->DirtyMap = true;
	}

	//need to regenerate it if region map is dirty
	if (this->DirtyMap) {
		(*this->Factory)({
			.RegionCount = this->RegionCount
		}, this->Map);
		this->DirtyMap = { };
	}
}

void FilterRunner::setRegionCount(const size_t region_count) noexcept {
	this->RegionCount = region_count;
	this->DirtyMap = true;
}

void FilterRunner::setFactory(const RegionMapFactory& factory) noexcept {
	this->Factory = &factory;
	this->DirtyMap = true;
}

void FilterRunner::sweepRadius(const SizeVec2& extent, const span<const Radius_t> radius_arr) {
	this->refreshMap(extent, *max_element(radius_arr));

	nb::Bench bench = FilterRunner::createBenchmark();
	bench.title("Time-Radius")
		.context("RegionCount", ::toString(this->Map.RegionCount).data())
		.context("Extent", ::formatSize(extent));

	RegionMapFilter::LaunchDescription desc {
		.Map = &this->Map,
		.Extent = extent
	};
	const auto run = [&desc, &histogram = this->Histogram, &filter = *this->Filter]() -> void {
		nb::doNotOptimizeAway(filter.filter(desc, histogram));
	};
	for (const auto r : radius_arr) {
		desc.Radius = r;
		desc.Offset = Utility::calcMinimumOffset(r);
		this->Filter->tryAllocateHistogram(desc, this->Histogram);

		bench.run(::toString(r).data(), run);
	}

	this->renderReport(bench);
}

void FilterRunner::sweepRegionCount(const SizeVec2& extent, const Radius_t radius,
	const span<const size_t> region_count_arr) {

	nb::Bench bench = FilterRunner::createBenchmark();
	bench.title("Time-RegionCount")
		.context("Extent", ::formatSize(extent))
		.context("Radius", ::toString(radius).data());

	const RegionMapFilter::LaunchDescription desc {
		.Map = &this->Map,
		.Offset = Utility::calcMinimumOffset(radius),
		.Extent = extent,
		.Radius = radius
	};
	const auto run = [&desc, &histogram = this->Histogram, &filter = *this->Filter]() -> void {
		nb::doNotOptimizeAway(filter.filter(desc, histogram));
	};
	for (const auto rc : region_count_arr) {
		this->setRegionCount(rc);
		//shouldn't trigger reallocation because size does not change
		this->refreshMap(extent, radius);
		this->Filter->tryAllocateHistogram(desc, this->Histogram);

		bench.run(::toString(rc).data(), run);
	}

	this->renderReport(bench);
}