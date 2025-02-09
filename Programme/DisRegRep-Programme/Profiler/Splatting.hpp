#pragma once

#include <DisRegRep/Container/Regionfield.hpp>

#include <DisRegRep/Core/System/ProcessThreadControl.hpp>
#include <DisRegRep/Core/ThreadPool.hpp>

#include <DisRegRep/RegionfieldGenerator/Base.hpp>
#include <DisRegRep/RegionfieldGenerator/VoronoiDiagram.hpp>

#include <DisRegRep/Splatting/Convolution/Base.hpp>
#include <DisRegRep/Splatting/Base.hpp>

#include <span>
#include <string_view>
#include <tuple>

#include <memory>

#include <filesystem>

#include <cstdint>

namespace DisRegRep::Programme::Profiler {

/**
 * @brief Profiler for different region feature splatting methods. Splatting profiler runs all profiling works on separate threads, and
 * they retain the ownership of all array memory until the profiler is synchronised.
 */
class Splatting {
public:

	using Base = DisRegRep::Splatting::Base;
	using BaseConvolution = DisRegRep::Splatting::Convolution::Base;
	using DimensionType = Base::DimensionType;
	using KernelSizeType = BaseConvolution::KernelSizeType;

	using RegionCountType = Container::Regionfield::ValueType;

	using SeedType = RegionfieldGenerator::Base::SeedType;
	using CentroidCountType = RegionfieldGenerator::VoronoiDiagram::SizeType;

	using SizeType = std::uint_fast8_t;

	/**
	 * @brief To ensure an accurate runtime measurement, it is recommended that:
	 * - Number of thread should be set to between 1/3 and 1/2 of the total number of threads available to the system. Too few threads
	 * would prolong the profiling run and too many may cause resource contention and OS scheduling overhead.
	 * - Nevertheless, thread pool size should not exceed the number of bits set in the affinity mask, otherwise it would cause
	 * contention within the thread pool.
	 * - Thread affinities are interleaved to a separate core if running on a system with simultaneous multithreading enabled, to avoid
	 * having the same core running on multiple threads; or even simpler, disable SMT.
	 * - Thread affinities are set to so that they would not use any of the energy-efficient core if running on a system with
	 * heterogeneous processors.
	 *
	 * @note Setting the right thread affinity is especially importance if the executing system is running on any of the Intel Core
	 * 12th Gen and beyond due to the P/E-Core design. Utilise the P/E-Core architecture strategically!
	 */
	struct ThreadPoolCreateInfo {

		Core::ThreadPool::SizeType Size;
		Core::System::ProcessThreadControl::AffinityMask AffinityMask;

	};

	struct CommonSweepInfo {

		std::string_view Tag; /**< A custom tag that contains application-dependent info for identifying a result. */
		DimensionType Extent; /**< @link Base::InvokeInfo::Extent. */

	};

	struct RadiusSweepInfo {

		const CommonSweepInfo* CommonSweepInfo_;
		/**
		 * @brief Region count shall be set by the application and fixed during profiling. Regionfield matrices are automatically
		 * allocated with the appropriate size and region identifiers are generated with each of the pairing regionfield generator.
		 */
		std::tuple<
			std::span<const RegionfieldGenerator::Base* const>,
			std::span<Container::Regionfield* const>
		> Input;

	};

	struct RegionCountSweepInfo {

		const CommonSweepInfo* CommonSweepInfo_;
		std::span<const RegionfieldGenerator::Base* const> Input;

	};

	struct CentroidCountSweepInfo {

		const CommonSweepInfo* CommonSweepInfo_;
		SeedType Seed; /**< @link RegionfieldGenerator::Base::Seed. */
		RegionCountType RegionCount; /**< @link Container::Regionfield::RegionCount. */

	};

private:

	class Impl;
	std::unique_ptr<Impl> Impl_;

public:

	/**
	 * @brief Create a splatting profiler.
	 *
	 * @param result_dir Directory where profiler results are stored.
	 * @param thread_pool_info @link ThreadPoolCreateInfo.
	 *
	 * @exception std::filesystem::filesystem_error If `result_dir` does not exist.
	 */
	Splatting(std::filesystem::path, const ThreadPoolCreateInfo&);

	Splatting(const Splatting&) = delete;

	constexpr Splatting(Splatting&&) noexcept = default;

	Splatting& operator=(const Splatting&) = delete;

	constexpr Splatting& operator=(Splatting&&) noexcept = default;

	~Splatting();

	/**
	 * @brief Block the current thread until all profiler threads finish. All object lifetime requirements are lifted once this call
	 * returns.
	 */
	void synchronise() const;

	/**
	 * @brief Profile the impact of runtime by varying radius on a convolution-based splatting. Profiler will be executed by the order
	 * of the cartesian product of $radius \times splat \times std::apply(std::views::zip, info.Input)$.
	 *
	 * @param splat_conv This is passed in as a flattened column-major 2D matrix of size (radius * splat), where *radius* is a sequence
	 * of radii to be swept, and *splat* is each instance of convolution-based splatting to be profiled. As profiler is multithreaded,
	 * the application is responsible for setting the profiling radii for each instance.
	 * @param splat_size Number of *splat* instances in `splat_conv`.
	 * @param info @link RadiusSweepInfo.
	 */
	void sweepRadius(std::span<const BaseConvolution* const>, SizeType, const RadiusSweepInfo&) const;

	/**
	 * @brief Profile the impact of runtime by varying the number of regions on a regionfield, though it is not guaranteed that all
	 * regions are actually presented on the regionfield, depends on the generator used. Regionfield memory is managed internally and
	 * will be generated with the provided regionfield generator. Profiler will be executed by the order of the cartesian product of
	 * $splat \times region_count \times info.Input$.
	 *
	 * @param splat Splatting to be profiled.
	 * @param region_count Region count to be run in order.
	 * @param info @link RegionCountSweepInfo.
	 */
	void sweepRegionCount(std::span<const Base* const>, std::span<const RegionCountType>, const RegionCountSweepInfo&) const;

	/**
	 * @brief Profile the impact of runtime by varying the number of regions presented on a regionfield. Similar to @link
	 * sweepRegionCount, regionfield is automatically generated. Profiler will be executed by the order of the cartesian product of
	 * $splat \times centroid_count \times info.Input$.
	 *
	 * @param splat Splatting to be profiled.
	 * @param centroid_count Centroid count @link RegionfieldGenerator::VoronoiDiagram::CentroidCount to be run in order.
	 * @param info @link CentroidCountSweepInfo.
	 */
	void sweepCentroidCount(std::span<const Base* const>, std::span<const CentroidCountType>, const CentroidCountSweepInfo&) const;

};

}