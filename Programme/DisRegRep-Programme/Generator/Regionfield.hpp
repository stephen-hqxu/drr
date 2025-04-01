#pragma once

#include <DisRegRep/Container/Regionfield.hpp>

#include <DisRegRep/RegionfieldGenerator/Base.hpp>
#include <DisRegRep/RegionfieldGenerator/VoronoiDiagram.hpp>

/**
 * @brief Generate a regionfield using one of the stock regionfield generators.
 */
namespace DisRegRep::Programme::Generator::Regionfield {

/**
 * @brief Common settings shared by different regionfield generators.
 */
struct GenerateInfo {

	Container::Regionfield::DimensionType Resolution;
	Container::Regionfield::ValueType RegionCount;

	RegionfieldGenerator::Base::SeedType Seed;

};

/**
 * @brief Generate a regionfield with uniform regionfield generator.
 *
 * @param gen_info @link GenerateInfo.
 *
 * @return Regionfield generated based on uniform random distribution.
 */
[[nodiscard]] Container::Regionfield uniform(const GenerateInfo&);

/**
 * @brief Generate a regionfield with a Voronoi Diagram regionfield generator.
 *
 * @param gen_info @link GenerateInfo.
 * @param centroid_count @link RegionfieldGenerator::VoronoiDiagram::CentroidCount.
 *
 * @return Regionfield generated using Voronoi Diagram.
 */
[[nodiscard]] Container::Regionfield voronoiDiagram(const GenerateInfo&, RegionfieldGenerator::VoronoiDiagram::SizeType);

}