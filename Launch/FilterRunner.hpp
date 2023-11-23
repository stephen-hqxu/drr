#pragma once

#include <DisRegRep/Format.hpp>
#include <DisRegRep/ThreadPool.hpp>
#include <DisRegRep/Container/RegionMap.hpp>

#include <DisRegRep/Factory/RegionMapFactory.hpp>
#include <DisRegRep/Filter/RegionMapFilter.hpp>

#include "Utility.hpp"

#include <string_view>
#include <span>
#include <array>
#include <vector>
#include <any>

#include <filesystem>

namespace DisRegRep::Launch {

/**
 * @brief Run and benchmark filter.
*/
class FilterRunner {
public:

	/**
	 * @brief Information for sweeping a variable in a filter run.
	*/
	struct SweepDescription {

		const RegionMapFactory* Factory;
		std::span<const RegionMapFilter* const> Filter;

		std::string_view UserTag;

	};

private:

	//Used for running a single invocation of filter.
	struct RunDescription {

		const RegionMapFactory& Factory;
		const RegionMapFilter& Filter;
		const std::string_view& UserTag;

		RegionMap& Map;
		std::any& Histogram;

	};

	constexpr static size_t ThreadCount = 3u;

	std::filesystem::path ReportRoot;

	std::array<RegionMap, ThreadCount> Map;
	std::array<std::array<std::any, Utility::AllFilterTagSize>, ThreadCount> Histogram;/**< One histogram per thread per tag. */

	ThreadPool Worker;

	std::vector<std::future<void>> PendingTask;

	//Create a new report file from benchmark.
	template<typename Tag>
	void renderReport(Tag, const RunDescription&, auto&) const;

	template<typename Func>
	void runFilter(const Func&, const SweepDescription&);

public:

	/**
	 * @brief Initialise a filter runner.
	 * 
	 * @param test_report_dir The directory where test reports are stored.
	 * It's possible to place this directory with a common root path that is shared with other filter runners,
	 * this constructor ensures no filesystem race occurs.
	*/
	FilterRunner(const std::filesystem::path&);

	FilterRunner(const FilterRunner&) = delete;

	FilterRunner(FilterRunner&&) = delete;

	~FilterRunner() = default;

	/**
	 * @brief Profile impact of runtime by varying radius.
	 * 
	 * @param desc The description about the sweep.
	 * @param extent The extent of filter run area.
	 * @param radius_arr An array of radii to be run in order.
	 * @param region_count The number of region to use.
	*/
	void sweepRadius(const SweepDescription&, const Format::SizeVec2&,
		std::span<const Format::Radius_t>, Format::Region_t);

	/**
	 * @brief Profile impact of runtime by varying region count.
	 * Region map will be automatically regenerated using different region count.
	 * 
	 * @param desc The description about the sweep.
	 * @param extent The size of the filter area.
	 * @param radius The radius of filter kernel.
	 * @param region_count_arr An array of region count.
	*/
	void sweepRegionCount(const SweepDescription&, const Format::SizeVec2&,
		Format::Radius_t, std::span<const Format::Region_t>);

};

}