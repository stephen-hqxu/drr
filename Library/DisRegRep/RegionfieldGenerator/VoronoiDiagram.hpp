#pragma once

#include "Base.hpp"

#include <DisRegRep/Container/Regionfield.hpp>
#include <DisRegRep/Core/UninitialisedAllocator.hpp>

#include <glm/vec2.hpp>

#include <vector>

#include <cstdint>

namespace DisRegRep::RegionfieldGenerator {

/**
 * @brief Generate a regionfield where each region are placed in a Voronoi cell.
*/
class VoronoiDiagram final : public Base {
public:

	using SizeType = std::uint_fast16_t;

	SizeType CentroidCount = 1U; /**< Number of centroids on the Voronoi Diagram. */

private:

	using CoordinateType = glm::vec<2, std::uint_least16_t>;
	using AssignmentType = Container::Regionfield::ValueType;

	mutable std::vector<CoordinateType, Core::UninitialisedAllocator<CoordinateType>> Centroid;
	mutable std::vector<AssignmentType, Core::UninitialisedAllocator<AssignmentType>> Assignment;

	DRR_REGIONFIELD_GENERATOR_DECLARE_DELEGATING_FUNCTOR_IMPL;

public:

	constexpr VoronoiDiagram() noexcept = default;

	VoronoiDiagram(const VoronoiDiagram&) = delete;

	VoronoiDiagram(VoronoiDiagram&&) = delete;

	VoronoiDiagram& operator=(const VoronoiDiagram&) = delete;

	VoronoiDiagram& operator=(VoronoiDiagram&&) = delete;

	constexpr ~VoronoiDiagram() override = default;

	DRR_REGIONFIELD_GENERATOR_SET_INFO("Voronoi")

	DRR_REGIONFIELD_GENERATOR_DECLARE_FUNCTOR_ALL_IMPL;

};

}