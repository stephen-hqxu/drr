#pragma once

#include "Base.hpp"

#include <DisRegRep/Container/Regionfield.hpp>

#include <string_view>

#include <cstddef>

namespace DisRegRep::RegionfieldGenerator {

/**
 * @brief Generate a regionfield where each region are placed in a Voronoi cell.
*/
class VoronoiDiagram final : public Base {
public:

	std::size_t CentroidCount {}; /**< Number of centroids in the Voronoi Diagram. */

	constexpr VoronoiDiagram() noexcept = default;

	constexpr ~VoronoiDiagram() override = default;

	[[nodiscard]] constexpr std::string_view name() const noexcept override {
		return "Voronoi";
	}

	void operator()(Container::Regionfield&) const override;

};

}