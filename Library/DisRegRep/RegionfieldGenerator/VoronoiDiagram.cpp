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
#include <tuple>

#include <algorithm>
#include <iterator>
#include <ranges>

#include <random>
#include <utility>

using DisRegRep::RegionfieldGenerator::VoronoiDiagram;
using DisRegRep::Container::Regionfield, DisRegRep::Core::XXHash::RandomEngine;

using glm::f32vec2;

using std::array;
using std::ranges::min_element, std::ranges::distance,
	std::views::iota, std::views::cartesian_product;
using std::uniform_int_distribution,
	std::integer_sequence, std::make_integer_sequence;

DRR_REGIONFIELD_GENERATOR_DEFINE_DELEGATING_FUNCTOR(VoronoiDiagram) {
	DRR_ASSERT(this->CentroidCount > 0U);

	using DimensionType = Regionfield::DimensionType;
	using LengthType = DimensionType::length_type;
	const DimensionType rf_extent = regionfield.extent();
	const Core::XXHash::Secret secret = VoronoiDiagram::generateSecret(gen_info);

	this->Centroid.assign_range(Core::View::Generate([
		dist = [rf_extent]<LengthType... I>(integer_sequence<LengthType, I...>) {
			return array { uniform_int_distribution<CoordinateType::value_type>(0U, rf_extent[I] - 1U)... };
		}(make_integer_sequence<LengthType, DimensionType::length()> {}),
		rng = RandomEngine(secret)
	] mutable {
		return std::apply([&rng](auto&... dist_n) { return CoordinateType(dist_n(rng)...); }, dist);
	}, this->CentroidCount));
	this->Assignment.assign_range(this->Centroid
		| Core::View::IrregularTransform(
			[dist = VoronoiDiagram::createDistribution(regionfield), &secret](const auto centroid) mutable {
				auto rng = RandomEngine(secret, auto(centroid));
				return dist(rng);
			}));

	//Find the nearest centroid for every point in the regionfield.
	//This is a pretty Naive algorithm, in practice it is better to use a quad tree or KD tree to find K-NN;
	//	omitted here for simplicity.
	const auto idx_rg = [rf_extent]<LengthType... I>(integer_sequence<LengthType, I...>) constexpr noexcept {
		return cartesian_product(iota(DimensionType::value_type {}, rf_extent[I])...)
			| Core::View::Functional::MakeFromTuple<f32vec2>;
	}(make_integer_sequence<LengthType, DimensionType::length()> {});
	std::transform(EpTrait::Unsequenced, idx_rg.cbegin(), idx_rg.cend(), regionfield.span().begin(), [this](const auto idx) noexcept {
		const auto argmin = distance(
			this->Centroid.cbegin(), min_element(this->Centroid, {}, [current_coord = idx](const f32vec2 centroid_coord) noexcept {
				return glm::distance(current_coord, centroid_coord);
			}));
		return this->Assignment[argmin];
	});
}

DRR_REGIONFIELD_GENERATOR_DEFINE_FUNCTOR_ALL(VoronoiDiagram)