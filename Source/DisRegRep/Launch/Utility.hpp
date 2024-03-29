#pragma once

#include <DisRegRep/Format.hpp>
#include <DisRegRep/Factory/RegionMapFactory.hpp>
#include <DisRegRep/Filter/RegionMapFilter.hpp>
#include <DisRegRep/Container/RegionMap.hpp>

#include <string>
#include <array>
#include <tuple>

#include <algorithm>

#include <chrono>
#include <utility>
#include <format>

namespace DisRegRep::Launch {

/**
 * @brief Some handy tools...
*/
namespace Utility {

//Some handy type aliases
using RMFT = DisRegRep::RegionMapFilter::LaunchTag;
constexpr std::tuple<RMFT::DCacheDHist, RMFT::DCacheSHist, RMFT::SCacheSHist> AllFilterTag { };
constexpr size_t AllFilterTagSize = std::tuple_size_v<decltype(AllFilterTag)>;

using TimestampClock = std::chrono::system_clock;
using Timestamp = TimestampClock::time_point;

/**
 * @brief Calculate the minimum region map dimension.
 * 
 * @param extent The filter area extent.
 * @param radius The radius of kernel.
 * 
 * @return The minimum region map dimension.
*/
constexpr Format::SizeVec2 calcMinimumDimension(Format::SizeVec2 extent, const Format::Radius_t radius) {
	std::ranges::transform(extent, extent.begin(), [radius](const auto ext) constexpr noexcept { return ext + 2u * radius; });
	return extent;
}

/**
 * @brief Calculate the minimum region map offset for radius.
 * 
 * @param radius Specify radius of filter.
 * 
 * @return The minimum region map offset.
*/
constexpr Format::SizeVec2 calcMinimumOffset(const Format::Radius_t radius) {
	Format::SizeVec2 offset { };
	offset.fill(radius);
	return offset;
}

/**
 * @brief Convert an array of printable values to a string.
 * 
 * @tparam T The type of array.
 * @tparam S The number of element.
 * @param arr The array to be stringified.
 * 
 * @return The string representation.
*/
template<typename T, size_t S>
constexpr std::string formatArray(const std::array<T, S>& arr) {
	std::string output;
	const auto push_value = [&arr, &output]<size_t... I>(std::index_sequence<I...>) constexpr -> void {
		((output.append(std::to_string(arr[I])), output.push_back(',')), ...);
	};
	output.push_back('(');
	push_value(std::make_index_sequence<S> { });
	output.push_back(')');
	return output;
}

/**
 * @brief Print a timestamp in string.
 * 
 * @param timestamp The timestamp to be printed.
 * 
 * @return The string representation of the timestamp.
*/
inline std::string formatTimestamp(const Timestamp& timestamp) {
	using namespace std::chrono;
	const auto now = zoned_time(current_zone(), timestamp).get_local_time();
	return std::format("{:%Y-%m-%d_%H-%M}", now);
}

/**
 * @brief Generate a smallest region map that satisfies the requirement.
 * 
 * @param region_map The region map output.
 * @param factory The region map factory.
 * @param extent The expected extent of filtering.
 * @param radius The expected radius of filtering kernel.
 * @param region_count The number of region.
*/
inline void generateMinimumRegionMap(RegionMap& region_map, const RegionMapFactory& factory,
	const Format::SizeVec2& extent, const Format::Radius_t radius, const Format::Region_t region_count) {
	const Format::SizeVec2 new_dimension = calcMinimumDimension(extent, radius);
	RegionMapFactory::reshape(region_map, new_dimension);
	factory({
		.RegionCount = region_count
	}, region_map);
}

}

}