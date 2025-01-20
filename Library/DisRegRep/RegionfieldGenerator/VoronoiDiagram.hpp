#pragma once

#include "Base.hpp"

#include <DisRegRep/Container/Regionfield.hpp>

#include <string_view>

#include <cstdint>

namespace DisRegRep::RegionfieldGenerator {

/**
 * @brief Generate a regionfield where each region are placed in a Voronoi cell.
*/
class VoronoiDiagram final : public Base {
public:

	using SizeType = std::uint_fast16_t;

	SizeType CentroidCount {}; /**< Number of centroids in the Voronoi Diagram. */

	constexpr VoronoiDiagram() noexcept = default;

	constexpr ~VoronoiDiagram() override = default;

	[[nodiscard]] constexpr std::string_view name() const noexcept override {
		return "Voronoi";
	}

	void operator()(Container::Regionfield&) const override;

};

}