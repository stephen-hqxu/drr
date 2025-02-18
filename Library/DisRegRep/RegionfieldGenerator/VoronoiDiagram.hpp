#pragma once

#include "Base.hpp"

#include <cstdint>

namespace DisRegRep::RegionfieldGenerator {

/**
 * @brief Generate a regionfield where each region are placed in a Voronoi cell.
*/
class VoronoiDiagram final : public Base {
public:

	using SizeType = std::uint_fast16_t;

private:

	DRR_REGIONFIELD_GENERATOR_DECLARE_DELEGATING_FUNCTOR_IMPL;

public:

	SizeType CentroidCount {}; /**< Number of centroids in the Voronoi Diagram. */

	DRR_REGIONFIELD_GENERATOR_SET_INFO("Voronoi")

	DRR_REGIONFIELD_GENERATOR_DECLARE_FUNCTOR_ALL_IMPL;

};

}