#pragma once

#include <DisRegRep/Format.hpp>
#include <DisRegRep/Factory/RegionMapFactory.hpp>
#include <DisRegRep/Filter/RegionMapFilter.hpp>

#include <string_view>
#include <span>

#include <filesystem>
#include <fstream>

namespace DisRegRep::Launch {

/**
 * @brief Run and benchmark filter.
*/
class FilterRunner {
private:

	std::filesystem::path ReportRoot;

	//Create a new benchmark.
	static auto createBenchmark();

	//Create a new report file from benchmark.
	void renderReport(auto& bench);

public:

	//Set filter to be used by the next run.
	const RegionMapFilter* Filter;

	/**
	 * @brief Initialise a filter runner.
	 * 
	 * @param test_report_dir The directory where test reports are stored.
	*/
	FilterRunner(std::string_view test_report_dir);

	FilterRunner(const FilterRunner&) = delete;

	FilterRunner(FilterRunner&&) = delete;

	~FilterRunner() = default;

	/**
	 * @brief Profile impact of runtime by varying radius.
	 * 
	 * @param region_map The region map to be used.
	 * It must be sufficiently large to perform filtering on all given radii.
	 * @param extent The extent of filter run area.
	 * @param radius_arr An array of radii to be run in order.
	*/
	void sweepRadius(const Format::RegionMap& region_map, const Format::SizeVec2& extent,
		std::span<const Format::Radius_t> radius_arr);

	/**
	 * @brief Profile impact of runtime by varying region count.
	 * 
	 * @param map_factory The region map factory to be used.
	 * @param extent The size of the filter area.
	 * @param radius The radius of filter kernel.
	 * @param region_count_arr An array of region count.
	*/
	void sweepRegionCount(const RegionMapFactory& map_factory, const Format::SizeVec2& extent,
		Format::Radius_t radius, std::span<const size_t> region_count_arr);

};

}