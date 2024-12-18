#include <DisRegRep/RegionfieldGenerator/Uniform.hpp>

#include <DisRegRep/Container/Regionfield.hpp>

#include <algorithm>
#include <functional>

using DisRegRep::RegionfieldGenerator::Uniform, DisRegRep::Container::Regionfield;

using std::ranges::generate, std::bind_front;

void Uniform::operator()(Regionfield& regionfield) {
	generate(regionfield.span(), bind_front(Base::createDistribution(regionfield), this->createRandomEngine()));
}