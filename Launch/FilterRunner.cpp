//WORKAROUND: MSVC's bug for not having constexpr mutex by default,
//	their documentation says it will be fixed in 17.9
#define _ENABLE_CONSTEXPR_MUTEX_CONSTRUCTOR

#include "FilterRunner.hpp"
#include "Utility.hpp"

#include <nb/nanobench.h>

#include <algorithm>
#include <functional>

#include <format>
#include <fstream>
#include <stdexcept>

#include <mutex>

#include <charconv>
#include <concepts>
#include <limits>
#include <utility>

using std::ranges::max_element;

using std::string_view, std::span, std::array, std::any;
using std::to_chars;
using std::ofstream, std::runtime_error, std::format;
using std::tuple;

namespace fs = std::filesystem;
namespace nb = ankerl::nanobench;
using namespace DisRegRep::Format;
using namespace DisRegRep::Launch;

namespace {

constexpr char RenderResultTemplate[] = R"DELIM(x,iter,t_median,t_mean,t_mdape,t_total
{{#result}}{{name}},{{sum(iterations)}},{{median(elapsed)}},{{average(elapsed)}},{{medianAbsolutePercentError(elapsed)}},{{sumProduct(elapsed,iterations)}}
{{/result}})DELIM";

constinit std::mutex GlobalFilesystemLock;

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

FilterRunner::FilterRunner(const fs::path& test_report_dir) :
	ReportRoot(test_report_dir), Factory(nullptr), RegionCount { }, DirtyMap(true), Filter(nullptr) {
	const auto fs_lock = std::unique_lock(::GlobalFilesystemLock);
	fs::create_directory(this->ReportRoot);
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

template<typename Tag>
void FilterRunner::renderReport(Tag, auto& bench) {
	this->renderReport(Tag::TagName, bench);
}

void FilterRunner::renderReport(const string_view& filter_tag, auto& bench) {
	string_view user_tag;
	if (this->UserTag.empty()) {
		user_tag = "-";
	} else {
		user_tag = this->UserTag;
	}

	const fs::path report_file = this->ReportRoot / format("{0}({2},{4},{1},{3}).csv", bench.title(),
		this->Filter->name(), this->Factory->name(), user_tag, filter_tag);

	auto csv = ofstream(report_file.string());
	bench.render(::RenderResultTemplate, csv);
}

inline void FilterRunner::refreshMap(const SizeVec2& new_extent, const Radius_t radius) {
	const SizeVec2 new_dimension = Utility::calcMinimumDimension(new_extent, radius);
	if (RegionMapFactory::reshape(this->Map, new_dimension)) {
		this->markRegionMapDirty();
	}

	//need to regenerate it if region map is dirty
	if (this->DirtyMap) {
		(*this->Factory)({
			.RegionCount = this->RegionCount
		}, this->Map);
		this->DirtyMap = { };
	}
}

template<typename Func>
void FilterRunner::runAllFilter(const Func& runner) {
	const auto run = [&runner, &hist_mem = this->Histogram]<size_t... I>(std::index_sequence<I...>) -> void {
		(std::invoke(runner, std::get<I>(Utility::AllFilterTag), hist_mem[I]), ...);
	};
	run(std::make_index_sequence<Utility::AllFilterTagSize> { });
}

void FilterRunner::setRegionCount(const Region_t region_count) noexcept {
	this->RegionCount = region_count;
	this->markRegionMapDirty();
}

void FilterRunner::setFactory(const RegionMapFactory& factory) noexcept {
	this->Factory = &factory;
	this->markRegionMapDirty();
}

void FilterRunner::markRegionMapDirty() noexcept {
	this->DirtyMap = true;
}

void FilterRunner::sweepRadius(const SizeVec2& extent, const span<const Radius_t> radius_arr) {
	this->refreshMap(extent, *max_element(radius_arr));
	RegionMapFilter::LaunchDescription desc {
		.Map = &this->Map,
		.Extent = extent
	};

	const auto run_radius = [this, &extent, &radius_arr, &desc]
		(const auto filter_tag, any& histogram) -> void {
		nb::Bench bench = FilterRunner::createBenchmark();
		bench.title("Time-Radius");

		const RegionMapFilter& filter = *this->Filter;
		const auto run = [&desc, &histogram, &filter, filter_tag]() -> void {
			nb::doNotOptimizeAway(filter(filter_tag, desc, histogram));
		};
		for (const auto r : radius_arr) {
			desc.Radius = r;
			desc.Offset = Utility::calcMinimumOffset(r);
			filter.tryAllocateHistogram(filter_tag, desc, histogram);

			bench.run(::toString(r).data(), run);
		}

		this->renderReport(filter_tag, bench);	
	};
	this->runAllFilter(run_radius);
}

void FilterRunner::sweepRegionCount(const SizeVec2& extent, const Radius_t radius,
	const span<const Region_t> region_count_arr) {
	const RegionMapFilter::LaunchDescription desc {
		.Map = &this->Map,
		.Offset = Utility::calcMinimumOffset(radius),
		.Extent = extent,
		.Radius = radius
	};

	const auto run_region_count = [this, &extent, radius, &region_count_arr, &desc]
		(const auto filter_tag, any& histogram) -> void {
		nb::Bench bench = FilterRunner::createBenchmark();
		bench.title("Time-RegionCount");

		const RegionMapFilter& filter = *this->Filter;
		const auto run = [&desc, &histogram, &filter, filter_tag]() -> void {
			nb::doNotOptimizeAway(filter(filter_tag, desc, histogram));
		};
		for (const auto rc : region_count_arr) {
			this->setRegionCount(rc);
			//shouldn't trigger reallocation because size does not change
			this->refreshMap(extent, radius);
			filter.tryAllocateHistogram(filter_tag, desc, histogram);

			bench.run(::toString(rc).data(), run);
		}

		this->renderReport(filter_tag, bench);
	};
	this->runAllFilter(run_region_count);

}