#pragma once

#include <DisRegRep/Format.hpp>
#include <DisRegRep/Factory/RegionMapFactory.hpp>
#include <DisRegRep/Filter/RegionMapFilter.hpp>

#include <string>
#include <string_view>
#include <span>
#include <any>

#include <filesystem>

namespace DisRegRep::Launch {

/**
 * @brief Run and benchmark filter.
*/
class FilterRunner {
private:

	std::filesystem::path ReportRoot;

	const RegionMapFactory* Factory;
	size_t RegionCount;

	bool DirtyMap;/**< If true, region map will be regenerated in the next execution. */
	Format::RegionMap Map;
	std::any Histogram;

	//Create a new benchmark.
	static auto createBenchmark();

	//Create a new report file from benchmark.
	void renderReport(auto& bench);

	//Refresh map dimension and content.
	void refreshMap(const Format::SizeVec2& new_extent, const Format::Radius_t radius);

public:

	//Set them to be used in the next filter execution.
	const RegionMapFilter* Filter;
	//Tag given by user to be added to the test report.
	std::string UserTag;

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
	 * @brief Set the region count of the internal region map.
	 * 
	 * @param region_count The new region count.
	*/
	void setRegionCount(const size_t region_count) noexcept;

	/**
	 * @brief Set the factory used for generating region map.
	 * 
	 * @param factory The factory.
	*/
	void setFactory(const RegionMapFactory& factory) noexcept;

	/**
	 * @brief Profile impact of runtime by varying radius.
	 * 
	 * @param region_map The region map to be used.
	 * It must be sufficiently large to perform filtering on all given radii.
	 * @param extent The extent of filter run area.
	 * @param radius_arr An array of radii to be run in order.
	*/
	void sweepRadius(const Format::SizeVec2& extent, std::span<const Format::Radius_t> radius_arr);

	/**
	 * @brief Profile impact of runtime by varying region count.
	 * Region map will be automatically regenerated using different region count.
	 * 
	 * @param map_factory The region map factory to be used.
	 * @param extent The size of the filter area.
	 * @param radius The radius of filter kernel.
	 * @param region_count_arr An array of region count.
	*/
	void sweepRegionCount(const Format::SizeVec2& extent, Format::Radius_t radius,
		std::span<const size_t> region_count_arr);

};

}