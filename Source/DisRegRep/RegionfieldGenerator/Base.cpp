#include <DisRegRep/RegionfieldGenerator/Base.hpp>

#include <DisRegRep/Container/Regionfield.hpp>

using DisRegRep::RegionfieldGenerator::Base, DisRegRep::Container::Regionfield;

Base::RandomEngineType Base::createRandomEngine() const noexcept {
	return RandomEngineType(this->Seed);
}

Base::UniformDistributionType Base::createDistribution(const Regionfield& regionfield) noexcept {
	return UniformDistributionType(0U, regionfield.regionCount() - 1U);
}