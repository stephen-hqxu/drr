#pragma once

#include "Splatting.hpp"

#include <string_view>
#include <tuple>

#include <type_traits>

/**
 * @brief Provide high-level access to all profilers.
 */
namespace DisRegRep::Programme::Profiler::Driver {

/**
 * @brief Define a linear range of values for a variable which specifies the start, stop and number of step of the sequence.
 * 
 * @tparam T Value type.
 */
template<typename T>
requires std::is_arithmetic_v<T>
using LinearSweepVariable = std::tuple<T, T, Splatting::SizeType>;

/**
 * @brief Splatting profiler provides two profiles:
 * - Default: Sweep all variables defined by the splatting profiler.
 * - Stress: Only applicable to radius sweep, mainly to profile the scalability of the optimised convolution kernel.
 * There are two different parameter sets within each profile:
 * - Fix: Controlled variables fixed throughout a specific run.
 * - Sweep: If the current run is sweeping a specific variable, the corresponding fixed variable is ignored.
 * Parameters placed output of these sets are invariant throughout the entire run.
 */
struct SplattingInfo {

	std::string_view ResultDirectory; /**< A new directory is created in this directory where all profiler outputs are stored. */
	const Splatting::ThreadPoolCreateInfo* ThreadPoolCreateInfo; /**< Please read this documentation carefully regarding how to tune the profiler. */

	Splatting::SeedType Seed;

	struct {

		struct {

			Splatting::DimensionType Extent;

			Splatting::KernelSizeType Radius;
			Splatting::RegionCountType RegionCount;
			Splatting::CentroidCountType CentroidCount;

		} Fixed;
		struct {

			LinearSweepVariable<Splatting::KernelSizeType> Radius;
			LinearSweepVariable<Splatting::RegionCountType> RegionCount;
			LinearSweepVariable<Splatting::CentroidCountType> CentroidCount;

		} Variable;

	} Default;
	struct {

		struct {

			Splatting::DimensionType Extent;

			Splatting::RegionCountType RegionCount;

		} Fixed;
		struct {

			LinearSweepVariable<Splatting::KernelSizeType> Radius;

		} Variable;

	} Stress;

};

/**
 * @brief Run splatting profiler @link Splatting.
 *
 * @param info @link SplattingInfo.
 */
void splatting(const SplattingInfo&);

}