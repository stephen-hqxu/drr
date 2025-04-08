#pragma once

#include <DisRegRep/Container/Regionfield.hpp>
#include <DisRegRep/Container/SplattingCoefficient.hpp>

#include <DisRegRep/RegionfieldGenerator/Base.hpp>
#include <DisRegRep/RegionfieldGenerator/VoronoiDiagram.hpp>

#include <DisRegRep/Splatting/Convolution/Full/Base.hpp>

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
 * @brief Region identifiers are distributed in a 2D space uniformly random.
 */
struct Uniform { };
/**
 * @brief Generate random coordinates for points to be used as cell centroids. Cell boundaries are constructed by finding the
 * perpendicular bisector of the line segment between two of the nearest centroids. Regions are then divided into such cells.
 */
struct VoronoiDiagram {

	RegionfieldGenerator::VoronoiDiagram::SizeType CentroidCount; /**< @link RegionfieldGenerator::VoronoiDiagram::CentroidCount. */

};

using Option = std::variant<
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
 * regionfield matrix. Every element in such a kernel is taken into account to derive the splatting coefficient.
 */
struct FullConvolutionOccupancy {

	DisRegRep::Splatting::Convolution::Full::Base::KernelSizeType Radius; /**< @link DisRegRep::Splatting::Convolution::Full::Base::Radius. */

};

using Option = std::variant<
	FullConvolutionOccupancy
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
 * @param splat_info @link SplattingInfo.
 * @param option Choose a region feature splatting coefficient algorithm.
 * @param regionfield Regionfield input that provides region identifiers whose splatting coefficients are to be computed.
 *
 * @return The computed dense region mask.
 */
[[nodiscard]] Container::SplattingCoefficient::DenseMask splat(const Splatting::Option&, const Container::Regionfield&);

}