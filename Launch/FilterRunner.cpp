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

using std::string, std::string_view, std::span, std::any, std::array;
using std::to_chars;
using std::ofstream, std::runtime_error, std::format;

using DC = DisRegRep::RegionMapFilter::DescriptionContext;

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
	ReportRoot(test_report_dir), Filter(nullptr) {
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
	const fs::path report_file = (this->ReportRoot / format("{}({}).csv", bench.title(), this->Filter->name()));

	auto csv = ofstream(report_file.string());
	bench.render(::RenderResultTemplate, csv);
}

void FilterRunner::sweepRadius(const RegionMap& region_map, const SizeVec2& extent, const span<const Radius_t> radius_arr) {
	nb::Bench bench = FilterRunner::createBenchmark();
	bench.title("Time-Radius")
		.context("RegionCount", ::toString(region_map.RegionCount).data())
		.context("Extent", ::formatSize(extent));

	RegionMapFilter::LaunchDescription desc {
		.Map = &region_map,
		.Extent = extent
	};
	const auto fill_desc = [&desc](const Radius_t r) -> void {
		desc.Radius = r;
		desc.Offset = Utility::calcMinimumOffset(r);
	};
	
	//try to reuse histogram if possible, we allocate histogram that is big enough to do all benchmark
	//need to determine if radius impacts histogram size
	if (this->Filter->context() & DC::Radius) {
		fill_desc(*max_element(radius_arr));
	}
	any histogram = this->Filter->allocateHistogram(desc);
	
	for (const auto r : radius_arr) {
		fill_desc(r);
		
		bench.run(::toString(r).data(), [&desc, &histogram, &filter = *this->Filter]() {
			nb::doNotOptimizeAway(filter.filter(desc, histogram));
		});
	}

	this->renderReport(bench);
}

void FilterRunner::sweepRegionCount(const RegionMapFactory& map_factory, const SizeVec2& extent,
	const Radius_t radius, const span<const size_t> region_count_arr) {
	nb::Bench bench = FilterRunner::createBenchmark();
	bench.title("Time-RegionCount")
		.context("Extent", ::formatSize(extent))
		.context("Radius", ::toString(radius).data());

	RegionMapFilter::LaunchDescription desc {
		.Offset = Utility::calcMinimumOffset(radius),
		.Extent = extent,
		.Radius = radius
	};
	const auto fill_desc = [&desc](const RegionMap& map) -> void {
		desc.Map = &map;
	};
	
	if (this->Filter->context() & DC::RegionCount) {
		//create a fake region map to preallocate memory
		fill_desc({
			.RegionCount = *max_element(region_count_arr)
		});
	}
	RegionMap region_map = map_factory.allocateRegionMap(Utility::calcMinimumDimension(extent, radius));
	any histogram = this->Filter->allocateHistogram(desc);

	for (const auto rc : region_count_arr) {
		map_factory(RegionMapFactory::CreateDescription {
			.RegionCount = rc
		}, region_map);
		fill_desc(region_map);

		bench.run(::toString(rc).data(), [&desc, &histogram, &filter = *this->Filter]() {
			nb::doNotOptimizeAway(filter.filter(desc, histogram));
		});
	}

	this->renderReport(bench);
}