#include <DisRegRep/RegionfieldGenerator/Uniform.hpp>

#include <DisRegRep/Container/Regionfield.hpp>
#include <DisRegRep/Core/XXHash.hpp>

#include <algorithm>
#include <execution>
#include <ranges>

#include <utility>

using DisRegRep::RegionfieldGenerator::Uniform,
	DisRegRep::Container::Regionfield, DisRegRep::Core::XXHash::RandomEngine;

using std::transform, std::execution::par_unseq,
	std::views::iota, std::views::common;
using std::as_const;

void Uniform::operator()(Regionfield& regionfield) {
	const auto span = regionfield.span();
	const auto idx_rg = iota(Regionfield::IndexType {}, span.size()) | common;
	transform(par_unseq, idx_rg.begin(), idx_rg.end(), span.begin(),
		[&rf = as_const(regionfield), secret = this->generateSecret()](const auto idx) {
			auto dist = Base::createDistribution(rf);
			auto rng = RandomEngine(secret, idx);
			return dist(rng);
		});
}