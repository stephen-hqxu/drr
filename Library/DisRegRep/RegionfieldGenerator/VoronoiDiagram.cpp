#include <DisRegRep/RegionfieldGenerator/VoronoiDiagram.hpp>

#include <DisRegRep/Container/Regionfield.hpp>

#include <DisRegRep/Core/View/Generate.hpp>
#include <DisRegRep/Core/Exception.hpp>
#include <DisRegRep/Core/Type.hpp>
#include <DisRegRep/Core/XXHash.hpp>

#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

#include <array>
#include <span>
#include <vector>

#include <tuple>

#include <algorithm>
#include <execution>
#include <iterator>
#include <ranges>

#include <utility>

#include <cstddef>

using DisRegRep::RegionfieldGenerator::VoronoiDiagram;

using glm::u16vec2, glm::f32vec2;

using std::array, std::span, std::vector;
using std::apply;
using std::ranges::min_element, std::ranges::distance, std::ranges::to,
	std::execution::par_unseq,
	std::views::iota, std::views::cartesian_product;
using std::index_sequence;

void VoronoiDiagram::operator()(Container::Regionfield& regionfield) const {
	DRR_ASSERT(this->CentroidCount > 0U);
	const span rf_span = regionfield.span();
	const Container::Regionfield::ExtentType& rf_extent = regionfield.mapping().extents();

	auto rng = Core::XXHash::RandomEngine(this->generateSecret());
	array<UniformDistributionType, 2U> dist;
	std::ranges::transform(iota(0U, dist.size()), dist.begin(),
		[&rf_extent](const auto ext) { return UniformDistributionType(0U, rf_extent.extent(ext) - 1U); });

	const auto region_centroid =
		Core::View::Generate(
			[&rng, &dist] { return apply([&rng](auto&... dist_n) { return u16vec2(dist_n(rng)...); }, dist); }, this->CentroidCount)
		| to<vector>();
	const auto region_assignment =
		Core::View::Generate([dist = Base::createDistribution(regionfield), &rng]() mutable { return dist(rng); }, this->CentroidCount)
		| to<vector<Core::Type::RegionIdentifier>>();

	/*
	 * Find the nearest centroid for every point in the regionfield.
	 * This is a pretty Naive algorithm, in practice it is better to use a quad tree or KD tree to find K-NN;
	 *	omitted here for simplicity.
	 */
	const auto idx_rg = [&rf_extent]<std::size_t... I>(index_sequence<I...>) constexpr noexcept {
		return cartesian_product(iota(0U, rf_extent.extent(I))...);
	}(index_sequence<1U, 0U> {});
	std::transform(par_unseq, idx_rg.cbegin(), idx_rg.cend(), rf_span.begin(),
		[&region_centroid, &region_assignment](const auto& idx) noexcept {
			const auto [y, x] = idx;
			const auto argmin = distance(region_centroid.cbegin(),
				min_element(region_centroid, {},
					[current_coord = f32vec2(x, y)](const f32vec2 centroid_coord) noexcept {
						return glm::distance(current_coord, centroid_coord);
					}));
			return region_assignment[argmin];
		});
}