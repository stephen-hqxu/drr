#pragma once

#include <DisRegRep/Container/Regionfield.hpp>

#include <DisRegRep/RegionfieldGenerator/Base.hpp>
#include <DisRegRep/RegionfieldGenerator/VoronoiDiagram.hpp>

#include <variant>

/**
 * @brief Generate a regionfield using one of the stock regionfield generators.
 */
namespace DisRegRep::Programme::Generator::Regionfield {

using DimensionType = Container::Regionfield::DimensionType;
using RegionCountType = Container::Regionfield::ValueType;
using SeedType = RegionfieldGenerator::Base::SeedType;

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
 * @brief Common settings shared by different regionfield generators.
 */
struct GenerateInfo {

	DimensionType Resolution;
	RegionCountType RegionCount;

	SeedType Seed;

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

}