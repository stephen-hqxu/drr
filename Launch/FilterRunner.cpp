#include "FilterRunner.hpp"
#include "Utility.hpp"

#include <string>
#include <array>

#include <algorithm>

#include <print>
#include <chrono>
#include <sstream>
#include <stdexcept>

#include <charconv>
#include <concepts>
#include <limits>

using std::ranges::max_element;

using std::string, std::string_view, std::span, std::any, std::array;
using std::to_chars;
using std::ofstream, std::ostringstream, std::runtime_error, std::print;

using DC = DisRegRep::RegionMapFilter::DescriptionContext;

namespace fs = std::filesystem;
namespace nb = ankerl::nanobench;
using namespace DisRegRep::Format;
using namespace DisRegRep::Launch;

namespace {

constexpr char RenderResultTemplate[] = R"DELIM(x,t_median,t_mean,t_mdape
{{#result}}{{name}},{{median(elapsed)}},{{average(elapsed)}},{{medianAbsolutePercentError(elapsed)}}
{{/result}})DELIM";

string formatSize(const SizeVec2& size) {
	ostringstream msg;
	const auto [x, y] = size;
	print(msg, "({}, {})", x, y);
	return msg.str();
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

FilterRunner::FilterRunner(const string_view test_report_dir) : ReportRoot(test_report_dir) {
	//canonical path will remove potential trailing slash
	this->ReportRoot = fs::weakly_canonical(this->ReportRoot);
	{
		using namespace std::chrono;
		const auto now = zoned_time(current_zone(), system_clock::now()).get_local_time();

		//add a timestamp to the output directory
		ostringstream timed_dir;
		print(timed_dir, "{}-{:%Y-%m-%d_%H-%M}", this->ReportRoot.filename().string(), now);
		this->ReportRoot.replace_filename(timed_dir.str());
	}
	if (!fs::create_directory(this->ReportRoot)) {
		ostringstream err;
		print(err, "Unable use directory \'{}\'.", this->ReportRoot.string());
		throw runtime_error(err.str());
	}

	using namespace std::chrono_literals;
	this->Benchmark.timeUnit(1ms, "ms").unit("run")
		.epochs(15u).minEpochTime(5ms).maxEpochTime(10s)
		.performanceCounters(false);
}

void FilterRunner::renderReport() {
	ostringstream filename;
	print(filename, "{}({}).csv", this->Benchmark.title(), this->Filter->name());
	const fs::path report_file = (this->ReportRoot / filename.str());

	auto csv = ofstream(report_file.string());
	this->Benchmark.render(::RenderResultTemplate, csv);
}

void FilterRunner::sweepRadius(const RegionMap& region_map, const SizeVec2& extent, const span<const Radius_t> radius_arr) {
	this->Benchmark.title("Time-Radius")
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
		
		this->Benchmark.run(::toString(r).data(), [&desc, &histogram, &filter = *this->Filter]() {
			nb::doNotOptimizeAway(filter.filter(desc, histogram));
		});
	}

	this->renderReport();
}

void FilterRunner::sweepRegionCount(const RegionMapFactory& map_factory, const SizeVec2& extent,
	const Radius_t radius, const span<const size_t> region_count_arr) {
	this->Benchmark.title("Time-RegionCount")
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

		this->Benchmark.run(::toString(rc).data(), [&desc, &histogram, &filter = *this->Filter]() {
			nb::doNotOptimizeAway(filter.filter(desc, histogram));
		});
	}

	this->renderReport();
}