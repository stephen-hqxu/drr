#pragma once

#include <DisRegRep/Container/Regionfield.hpp>
#include <DisRegRep/Container/SplattingCoefficient.hpp>

#include <DisRegRep/RegionfieldGenerator/Base.hpp>
#include <DisRegRep/RegionfieldGenerator/DiamondSquare.hpp>
#include <DisRegRep/RegionfieldGenerator/VoronoiDiagram.hpp>

#include <DisRegRep/Splatting/OccupancyConvolution/Sampled/Stochastic.hpp>
#include <DisRegRep/Splatting/OccupancyConvolution/Sampled/Stratified.hpp>
#include <DisRegRep/Splatting/OccupancyConvolution/Sampled/Systematic.hpp>
#include <DisRegRep/Splatting/OccupancyConvolution/Base.hpp>

#include <vector>

#include <optional>
#include <tuple>
#include <variant>

/**
 * @brief Process a regionfield using one of the stock regionfield processing methods, such as regionfield generator and splatting
 * algorithm.
 */
namespace DisRegRep::Programme::Generator::Regionfield {

/**
 * @brief Choose a regionfield generator to generate a regionfield matrix.
 */
namespace Generator {

/**
 * @brief The Diamond-Square algorithm, originally developed for procedural terrain generation, is employed to subdivide a 2D domain
 * into regions. In contradistinction to the configuration of Voronoi Diagram, the delineation of the region is not accomplished by
 * linear incisions, a methodology that is employed to engender a more natural aesthetic.
 */
struct DiamondSquare {

	using Generator = RegionfieldGenerator::DiamondSquare;

	Generator::DimensionType InitialExtent; /**< @link Generator::InitialExtent. */
	std::vector<Generator::SizeType> Iteration; /**< @link Generator::Iteration. */

};
/**
 * @brief Region identifiers are distributed in a 2D space uniformly random.
 */
struct Uniform { };
/**
 * @brief Generate random coordinates for points to be used as cell centroids. Cell boundaries are constructed by finding the
 * perpendicular bisector of the line segment between two of the nearest centroids. Regions are then divided into such cells.
 */
struct VoronoiDiagram {

	using Generator = RegionfieldGenerator::VoronoiDiagram;

	Generator::SizeType CentroidCount; /**< @link Generator::CentroidCount. */

};

using Option = std::variant<
	const DiamondSquare*,
	Uniform,
	VoronoiDiagram
>; /**< Supply configurations to all supported regionfield generators. */

}

/**
 * @brief Choose an algorithm for computing the region feature splatting coefficients.
 */
namespace Splatting {

/**
 * @brief The splatting coefficient is defined as the number of times each region identifier appears on the convolution kernel in a
 * regionfield matrix.
 */
namespace OccupancyConvolution {

/**
 * @brief Common settings shared by all occupancy convolution-based splatting.
 */
struct SplatInfo {

	DisRegRep::Splatting::OccupancyConvolution::Base::KernelSizeType
		Radius; /**< @link DisRegRep::Splatting::OccupancyConvolution::Base::Radius. */

};

/**
 * @brief Every element in such a kernel is taken into account to derive the splatting coefficient.
 */
struct Full { };

/**
 * @brief Elements in such a kernel are sampled with different strategies to derive the splatting coefficient.
 */
namespace Sampled {

/**
 * @brief Simple random sampling.
 */
struct Stochastic {

	using Splatting = DisRegRep::Splatting::OccupancyConvolution::Sampled::Stochastic;

	Splatting::KernelSizeType Sample; /**< @link Splatting::Sample. */
	Splatting::SeedType Seed; /**< @link Splatting::Seed. */

};
/**
 * @brief Stratified sampling.
 */
struct Stratified {

	using Splatting = DisRegRep::Splatting::OccupancyConvolution::Sampled::Stratified;

	Splatting::KernelSizeType StratumCount; /**< @link Splatting::StratumCount. */
	Splatting::SeedType Seed; /**< @link Splatting::Seed. */

};
/**
 * @brief Systematic sampling.
 */
struct Systematic {

	using Splatting = DisRegRep::Splatting::OccupancyConvolution::Sampled::Systematic;

	Splatting::DimensionType FirstSample, /**< @link Splatting::FirstSample. */
		Interval; /**< @link Splatting::Interval. */

};

}

using Option = std::variant<
	Full,
	const Sampled::Stochastic*,
	const Sampled::Stratified*,
	const Sampled::Systematic*
>; /**< All occupancy convolution-based splatting algorithms. */

}

using Option = std::variant<
	std::tuple<const OccupancyConvolution::SplatInfo*, OccupancyConvolution::Option>
>; /**< Supply configurations to all supported region feature splatting coefficient computing algorithms. */

}

/**
 * @brief Common settings shared by different regionfield generators.
 */
struct GenerateInfo {

	Container::Regionfield::DimensionType Resolution; /**< @link Container::Regionfield::resize. */
	Container::Regionfield::ValueType RegionCount; /**< @link Container::Regionfield::RegionCount. */

	RegionfieldGenerator::Base::GenerateInfo RegionfieldGeneratorGenerateInfo;

};

/**
 * @brief Common settings shared by different region feature splatting algorithms.
 */
struct SplatInfo {

	/**
	 * @brief Pass splatting invocation parameters to @link DisRegRep::Splatting::Base::InvokeInfo. Default to use the minimum
	 * requirements if not provided.
	 */
	std::optional<DisRegRep::Splatting::Base::DimensionType> Offset, Extent;

};

/**
 * @brief Generate a regionfield with a specific regionfield generator.
 *
 * @param gen_info @link GenerateInfo.
 * @param option Choose a regionfield generator and configure it.
 *
 * @return Regionfield generated using the specified generator.
 */
[[nodiscard]] Container::Regionfield generate(const GenerateInfo&, const Generator::Option&);

/**
 * @brief Compute region feature splatting coefficients for the whole domain of a given regionfield matrix.
 *
 * @param splat_info @link SplatInfo.
 * @param option Choose a region feature splatting coefficient algorithm.
 * @param regionfield Regionfield input that provides region identifiers whose splatting coefficients are to be computed.
 *
 * @return The computed dense region mask.
 */
[[nodiscard]] Container::SplattingCoefficient::DenseMask splat(
	const SplatInfo&, const Splatting::Option&, const Container::Regionfield&);

}