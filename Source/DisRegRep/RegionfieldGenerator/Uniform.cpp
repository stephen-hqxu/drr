#include <DisRegRep/RegionfieldGenerator/Uniform.hpp>

#include <DisRegRep/Container/Regionfield.hpp>

#include <algorithm>

using DisRegRep::RegionfieldGenerator::Uniform, DisRegRep::Container::Regionfield;

using std::ranges::generate;

void Uniform::operator()(Regionfield& regionfield) {
	generate(regionfield.span(),
		[rng = this->createRandomEngine(), dist = Base::createDistribution(regionfield)]() mutable { return dist(rng); });
}