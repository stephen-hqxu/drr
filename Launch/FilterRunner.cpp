//WORKAROUND: MSVC's bug for not having constexpr mutex by default,
//	their documentation says it will be fixed in 17.9
#define _ENABLE_CONSTEXPR_MUTEX_CONSTRUCTOR

#include "FilterRunner.hpp"

#include <DisRegRep/Factory/VoronoiRegionFactory.hpp>

#include <nb/nanobench.h>

#include <string_view>

#include <algorithm>
#include <ranges>
#include <functional>

#include <format>
#include <fstream>
#include <stdexcept>

#include <mutex>

#include <charconv>
#include <concepts>
#include <limits>
#include <utility>
#include <type_traits>

#include <cassert>

using std::ranges::for_each;

using std::string_view, std::span, std::array, std::any;
using std::to_chars, std::as_const, std::move;
using std::ofstream, std::runtime_error, std::format, std::unique_lock;

namespace fs = std::filesystem;
namespace nb = ankerl::nanobench;
using namespace DisRegRep::Format;
using namespace DisRegRep::Launch;

using DisRegRep::RegionMap, DisRegRep::RegionMapFactory;

namespace {

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

template<typename Tag>
nb::Bench createBenchmark(Tag) {
	using namespace std::chrono_literals;
	using std::is_same_v;

	nb::Bench bench;
	bench.timeUnit(1ms, "ms").unit("run")
		.epochs(15u).minEpochTime(5ms).maxEpochTime(10s)
		.performanceCounters(false)
		//need to disable console output because we are running with multiple threads.
		.output(nullptr);
	if constexpr (!is_same_v<Tag, Utility::RMFT::DCacheDHist>) {
		//sparse matrix requires incremental construction, and we are using dynamic array internally
		//so warm up helps preallocating memory.
		bench.warmup(1u);
	}
	return bench;
}

}

struct FilterRunner::RunDescription {

	const RegionMapFactory* Factory;
	const RegionMapFilter* Filter;
	std::string_view UserTag;

	RegionMap* Map;
	std::any* Histogram;
	RunResultSet* ResultSet;

	void tryAllocateHistogram(const auto filter_tag, const RegionMapFilter::LaunchDescription& desc) const {
		this->Filter->tryAllocateHistogram(filter_tag, desc, *this->Histogram);
	}

	template<typename Tag>
	void runFilter(const Tag filter_tag, const RegionMapFilter::LaunchDescription& desc,
		const typename Tag::HistogramType*& histogram_output) const {
		histogram_output = &(*this->Filter)(filter_tag, desc, *this->Histogram);
	}

	constexpr void recordRunResult(const RunResult& run_result) const {
		this->ResultSet->push_back(run_result);
	}

};

FilterRunner::FilterRunner(const fs::path& test_report_dir) :
	ReportRoot(test_report_dir), Worker(ThreadCount) {
	const auto fs_lock = unique_lock(::GlobalFilesystemLock);
	fs::create_directory(this->ReportRoot);
}

template<typename Tag>
void FilterRunner::renderReport(Tag, const RunDescription& desc, const auto& bench) const {
	const auto& [factory, filter, user_tag, _1, _2, result_set] = desc;

	const fs::path report_file = this->ReportRoot / format("{0}({2},{4},{1},{3}).csv", bench.title(),
		filter->name(), factory->name(), user_tag.empty() ? "Default" : user_tag, Tag::TagName);

	const auto lock = unique_lock(::GlobalFilesystemLock);
	auto csv = ofstream(report_file.string());
	print(csv, "x,iter,t_median,t_mean,t_mdape,t_total,memory");
	assert(result_set->size() == bench.results().size());
	for (const auto [bench_result, run_result] : std::views::zip(bench.results(), *result_set)) {
		using enum nb::Result::Measure;
		const nb::Config& config = bench_result.config();

		//time is in *ms*; memory is in *kB*
		print(csv, "\n{},{},{},{},{},{},{}",
			config.mBenchmarkName,
			bench_result.sum(iterations),
			bench_result.median(elapsed) * 1000.0,
			bench_result.average(elapsed) * 1000.0,
			bench_result.medianAbsolutePercentError(elapsed),
			bench_result.sumProduct(elapsed, iterations) * 1000.0,
			run_result.MemoryUsage * 0.0001
		);
	}
}

template<typename Func>
void FilterRunner::runFilter(Func&& runner, const SweepDescription& sweep_desc) {
	//It is very important to make sure to pass all captures by values
	//	since this function will be run from another thread.
	const auto invoke_runner = [this, runner = move(runner), sweep_desc](const ThreadPool::ThreadInfo& info,
		const auto tag, const size_t tag_idx, const size_t filter_idx) -> void {
		const auto& [factory, filter, user_tag] = sweep_desc;
			
		const auto [tid] = info;
		RunResultSet& result_set = this->Result[tid];
		result_set.clear();

		std::invoke(runner, tag, RunDescription {
			.Factory = factory,
			.Filter = filter[filter_idx],
			.UserTag = user_tag,

			.Map = &this->Map[tid],
			.Histogram = &this->Histogram[tid][tag_idx],
			.ResultSet = &result_set
		});
	};
	const auto enqueue_invoke_runner = [this, &invoke_runner]<size_t... I>(
		std::index_sequence<I...>, const size_t filter_idx) -> void {
		(this->PendingTask.emplace_back(this->Worker.enqueue(invoke_runner,
			std::get<I>(Utility::AllFilterTag), I, filter_idx)), ...);
	};
	for (const auto filter_idx : std::views::iota(size_t { 0 }, sweep_desc.Filter.size())) {
		enqueue_invoke_runner(std::make_index_sequence<Utility::AllFilterTagSize> { }, filter_idx);
	}
}

void FilterRunner::waitAll() {
	for_each(this->PendingTask, [](auto& future) { future.get(); });
	this->PendingTask.clear();
}

#define CREATE_BENCHMARK_RUN_FUNC [filter_tag, &run_desc, &desc, &histogram_output]() \
{ run_desc.runFilter(filter_tag, desc, histogram_output); }
//It is fine to take the memory usage of the last histogram, since each run is identical,
//	and filters are all deterministic.
#define TIME_FILTER_RUN(VARIABLE) const typename Tag::HistogramType* histogram_output; \
bench.run(::toString(VARIABLE).data(), CREATE_BENCHMARK_RUN_FUNC); \
run_desc.recordRunResult({ \
	.MemoryUsage = histogram_output->sizeByte() \
})

void FilterRunner::sweepRadius(const SweepDescription& desc, const RegionMap& region_map,
	const SizeVec2& extent, const span<const Radius_t> radius_arr) {
	auto run_radius = [this, &region_map, extent, radius_arr]
		<typename Tag>(const Tag filter_tag, const RunDescription run_desc) -> void {
		RegionMapFilter::LaunchDescription desc {
			.Map = &region_map,
			.Extent = extent
		};
		
		nb::Bench bench = ::createBenchmark(filter_tag);
		bench.title("Time-Radius");

		for (const auto r : radius_arr) {
			desc.Radius = r;
			desc.Offset = Utility::calcMinimumOffset(r);
			run_desc.tryAllocateHistogram(filter_tag, desc);

			TIME_FILTER_RUN(r);
		}

		this->renderReport(filter_tag, run_desc, bench);
	};
	this->runFilter(move(run_radius), desc);
}

void FilterRunner::sweepRegionCount(const SweepDescription& desc, const SizeVec2& extent,
	const Radius_t radius, const span<const Region_t> region_count_arr) {
	auto run_region_count = [this, extent, radius, region_count_arr]
		<typename Tag>(const Tag filter_tag, const RunDescription run_desc) -> void {
		const RegionMapFilter::LaunchDescription desc {
			.Map = run_desc.Map,
			.Offset = Utility::calcMinimumOffset(radius),
			.Extent = extent,
			.Radius = radius
		};

		nb::Bench bench = ::createBenchmark(filter_tag);
		bench.title("Time-RegionCount");

		for (const auto rc : region_count_arr) {
			//shouldn't trigger reallocation because size does not change
			Utility::generateMinimumRegionMap(*run_desc.Map, *run_desc.Factory, extent, radius, rc);
			run_desc.tryAllocateHistogram(filter_tag, desc);

			TIME_FILTER_RUN(rc);
		}

		this->renderReport(filter_tag, run_desc, bench);
	};
	this->runFilter(move(run_region_count), desc);

}

void FilterRunner::sweepCentroidCount(const SweepDescription& desc, const SizeVec2& extent,
	const Radius_t radius, const Region_t region_count, const std::uint64_t random_seed,
	const span<const size_t> centroid_count_arr) {
	auto run_centroid_count = [this, extent, radius, region_count, random_seed, centroid_count_arr]
		<typename Tag>(const Tag filter_tag, RunDescription run_desc) -> void {
		const RegionMapFilter::LaunchDescription desc {
			.Map = run_desc.Map,
			.Offset = Utility::calcMinimumOffset(radius),
			.Extent = extent,
			.Radius = radius
		};

		auto voronoi_factory = DisRegRep::VoronoiRegionFactory(random_seed);
		run_desc.Factory = &voronoi_factory;

		nb::Bench bench = ::createBenchmark(filter_tag);
		bench.title("Time-CentroidCount");

		for (const auto cc : centroid_count_arr) {
			voronoi_factory.CentroidCount = cc;
			Utility::generateMinimumRegionMap(*run_desc.Map, voronoi_factory, extent, radius, region_count);
			run_desc.tryAllocateHistogram(filter_tag, desc);

			TIME_FILTER_RUN(cc);
		}

		this->renderReport(filter_tag, run_desc, bench);
	};
	this->runFilter(move(run_centroid_count), desc);
}