#pragma once

#include "Splatting.hpp"

#include <yaml-cpp/yaml.h>

#include <tuple>

#include <filesystem>

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
 * - Fixed: Controlled parameters fixed throughout a specific run.
 * - Variable: If the current run is sweeping a specific variable, the corresponding fixed parameter is ignored.
 * Parameters placed output of these sets are invariant throughout the entire run.
 */
struct SplattingInfo {

	struct ParameterSetType {

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

	const std::filesystem::path* ResultDirectory; /**< A new directory is created in this directory where all profiler outputs are stored. */
	const Splatting::ThreadPoolCreateInfo* ThreadPoolCreateInfo; /**< Please read this documentation carefully regarding how to tune the profiler. */
	Splatting::SeedType Seed;

	const ParameterSetType* ParameterSet;

};

/**
 * @brief Run splatting profiler @link Splatting.
 *
 * @param info @link SplattingInfo.
 *
 * @exception std::filesystem::filesystem_error If @link SplattingInfo::ResultDirectory does not exist.
 */
void splatting(const SplattingInfo&);

}

namespace YAML {

template<typename T>
struct convert<DisRegRep::Programme::Profiler::Driver::LinearSweepVariable<T>> {

	using ConvertType = DisRegRep::Programme::Profiler::Driver::LinearSweepVariable<T>;

	static bool decode(const Node& node, ConvertType& variable) {
		using std::tuple_size_v, std::tuple_element_t;
		if (!(node.IsMap() && node.size() == tuple_size_v<ConvertType>)) [[unlikely]] {
			return false;
		}
		variable = {
			node["from"].as<T>(),
			node["to"].as<T>(),
			node["step"].as<tuple_element_t<2U, ConvertType>>()
		};
		return true;
	}

};

template<>
struct convert<DisRegRep::Programme::Profiler::Driver::SplattingInfo::ParameterSetType> {

	using ConvertType = DisRegRep::Programme::Profiler::Driver::SplattingInfo::ParameterSetType;

	static bool decode(const Node&, ConvertType&);

};

}