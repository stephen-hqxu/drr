#include <DisRegRep/RegionfieldGenerator/VoronoiDiagram.hpp>
#include <DisRegRep/RegionfieldGenerator/ImplementationHelper.hpp>

#include <DisRegRep/Container/Regionfield.hpp>

#include <DisRegRep/Core/View/Functional.hpp>
#include <DisRegRep/Core/View/Generate.hpp>
#include <DisRegRep/Core/View/IrregularTransform.hpp>
#include <DisRegRep/Core/Exception.hpp>
#include <DisRegRep/Core/XXHash.hpp>

#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

#include <array>
#include <span>
#include <vector>

#include <tuple>

#include <algorithm>
#include <iterator>
#include <ranges>

#include <utility>

using DisRegRep::RegionfieldGenerator::VoronoiDiagram;
using DisRegRep::Container::Regionfield, DisRegRep::Core::XXHash::RandomEngine;

using glm::u16vec2, glm::f32vec2;

using std::array, std::span, std::vector;
using std::apply;
using std::ranges::min_element, std::ranges::distance, std::ranges::to,
	std::views::iota, std::views::cartesian_product;
using std::integer_sequence, std::make_integer_sequence;

DRR_REGIONFIELD_GENERATOR_DEFINE_DELEGATING_FUNCTOR(VoronoiDiagram) {
	using RankType = Regionfield::ExtentType::rank_type;
	DRR_ASSERT(this->CentroidCount > 0U);

	const span rf_span = regionfield.span();
	const Regionfield::DimensionType rf_extent = regionfield.extent();

	array<UniformDistributionType, 2U> dist;
	std::ranges::transform(iota(RankType {}, static_cast<RankType>(dist.size())), dist.begin(),
		[&rf_extent](const auto ext) { return UniformDistributionType(0U, rf_extent[ext] - 1U); });

	const Core::XXHash::Secret secret = VoronoiDiagram::generateSecret(gen_info);
	const auto region_centroid = Core::View::Generate(
			[&dist, rng = RandomEngine(secret)] mutable {
				return apply([&rng](auto&... dist_n) { return u16vec2(dist_n(rng)...); }, dist);
			},
			this->CentroidCount
		) | to<vector>();
	const auto region_assignment = region_centroid
		| Core::View::IrregularTransform(
			[dist = VoronoiDiagram::createDistribution(regionfield), &secret](const auto& centroid) mutable {
				auto rng = RandomEngine(secret, centroid);
				return dist(rng);
			})
		| to<vector<Regionfield::ValueType>>();

	//Find the nearest centroid for every point in the regionfield.
	//This is a pretty Naive algorithm, in practice it is better to use a quad tree or KD tree to find K-NN;
	//	omitted here for simplicity.
	const auto idx_rg = [&rf_extent]<RankType... I>(integer_sequence<RankType, I...>) constexpr noexcept {
		return cartesian_product(iota(Regionfield::IndexType {}, rf_extent[I])...)
			 | Core::View::Functional::MakeFromTuple<Regionfield::DimensionType>;
	}(make_integer_sequence<RankType, Regionfield::ExtentType::rank()> {});
	std::transform(EpTrait::Unsequenced, idx_rg.cbegin(), idx_rg.cend(), rf_span.begin(),
		[&region_centroid, &region_assignment](const auto idx) noexcept {
			const auto argmin = distance(region_centroid.cbegin(),
				min_element(region_centroid, {}, [current_coord = f32vec2(idx)](const f32vec2 centroid_coord) noexcept {
					return glm::distance(current_coord, centroid_coord);
				}));
			return region_assignment[argmin];
		});
}

DRR_REGIONFIELD_GENERATOR_DEFINE_FUNCTOR_ALL(VoronoiDiagram)