#include <DisRegRep/RegionfieldGenerator/VoronoiDiagram.hpp>

#include <DisRegRep/Container/Regionfield.hpp>

#include <DisRegRep/Exception.hpp>
#include <DisRegRep/Type.hpp>

#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

#include <range/v3/view/generate_n.hpp>

#include <array>
#include <vector>

#include <algorithm>
#include <functional>
#include <execution>
#include <iterator>
#include <ranges>

using DisRegRep::RegionfieldGenerator::VoronoiDiagram, DisRegRep::Container::Regionfield;

using glm::vec, glm::f32vec2;

using ranges::views::generate_n;

using std::array, std::vector;
using std::execution::par_unseq;
using std::for_each, std::ranges::min_element, std::ranges::distance,
	std::bind_front;
using std::ranges::to,
	std::views::iota, std::views::cartesian_product;

void VoronoiDiagram::operator()(Regionfield& regionfield) {
	DRR_ASSERT(this->CentroidCount > 0U);
	const Regionfield::MdSpanType rf_mdspan = regionfield.mdspan();

	using RandomIntVec2 = vec<2U, RandomIntType>;

	auto rng = this->createRandomEngine();
	array<UniformDistributionType, 2U> dist;
	std::ranges::transform(iota(0U, dist.size()), dist.begin(),
		[&rf_mdspan](const auto ext) { return UniformDistributionType(0U, rf_mdspan.extent(ext) - 1U); });

	const auto region_centroid =
		generate_n(
			[&rng, &dist] {
				auto& [dist0, dist1] = dist;
				return RandomIntVec2(dist0(rng), dist1(rng));
			}, this->CentroidCount)
		| to<vector>();
	const auto region_assignment =
		generate_n(bind_front(Base::createDistribution(regionfield), rng), this->CentroidCount)
		| to<vector<Type::RegionIdentifier>>();

	/*
	 * Find the nearest centroid for every point in the regionfield.
	 * This is a pretty Naive algorithm, in practice it is better to use a quad tree or KD tree to find K-NN;
	 *	omitted here for simplicity.
	 */
	const auto it = cartesian_product(iota(0U, rf_mdspan.extent(1U)), iota(0U, rf_mdspan.extent(0U)));
	for_each(par_unseq, it.cbegin(), it.cend(), [&rf_mdspan, &region_centroid, &region_assignment](const auto& idx) noexcept {
		const auto [y, x] = idx;

		const auto centroid_distance =
			region_centroid | std::views::transform([current_coord = f32vec2(x, y)](const f32vec2 centroid_coord) noexcept {
				return glm::distance(current_coord, centroid_coord);
			});
		const auto argmin = distance(centroid_distance.cbegin(), min_element(centroid_distance));

		rf_mdspan[x, y] = region_assignment[argmin];
	});
}