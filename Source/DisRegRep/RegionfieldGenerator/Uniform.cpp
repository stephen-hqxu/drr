#include <DisRegRep/RegionfieldGenerator/Uniform.hpp>

#include <DisRegRep/Container/Regionfield.hpp>
#include <DisRegRep/Core/XXHash.hpp>

#include <algorithm>
#include <execution>
#include <ranges>

#include <utility>

using DisRegRep::RegionfieldGenerator::Uniform;

using std::transform, std::execution::par_unseq,
	std::views::iota, std::views::common;
using std::as_const;

void Uniform::operator()(Container::Regionfield& regionfield) {
	const auto span = regionfield.span();
	const auto idx_rg = iota(Container::Regionfield::IndexType {}, span.size()) | common;
	transform(par_unseq, idx_rg.begin(), idx_rg.end(), span.begin(),
		[&rf = as_const(regionfield), secret = this->generateSecret()](const auto idx) {
			auto dist = Base::createDistribution(rf);
			auto rng = Core::XXHash::RandomEngine(secret, idx);
			return dist(rng);
		});
}