#include <DisRegRep/RegionfieldGenerator/Uniform.hpp>
#include <DisRegRep/RegionfieldGenerator/ImplementationHelper.hpp>

#include <DisRegRep/Container/Regionfield.hpp>
#include <DisRegRep/Core/XXHash.hpp>

#include <algorithm>
#include <ranges>

#include <utility>

using DisRegRep::RegionfieldGenerator::Uniform;

using std::transform, std::views::iota, std::as_const;

DRR_REGIONFIELD_GENERATOR_DEFINE_DELEGATING_FUNCTOR(Uniform) {
	const auto span = regionfield.span();

	using IndexType = Container::Regionfield::IndexType;
	const auto idx_rg = iota(IndexType {}, static_cast<IndexType>(span.size()));
	transform(EpTrait::Unsequenced, idx_rg.begin(), idx_rg.end(), span.begin(),
		[&rf = as_const(regionfield), secret = Uniform::generateSecret(gen_info)](const auto idx) {
			auto dist = Uniform::createDistribution(rf);
			auto rng = Core::XXHash::RandomEngine(secret, auto(idx));
			return dist(rng);
		});
}

DRR_REGIONFIELD_GENERATOR_DEFINE_FUNCTOR_ALL(Uniform)