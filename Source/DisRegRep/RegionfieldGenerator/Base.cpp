#include <DisRegRep/RegionfieldGenerator/Base.hpp>

#include <DisRegRep/Container/Regionfield.hpp>

#include <DisRegRep/Exception.hpp>

using DisRegRep::RegionfieldGenerator::Base, DisRegRep::Container::Regionfield;

Base::RandomEngineType Base::createRandomEngine() const noexcept {
	return RandomEngineType(this->Seed);
}

Base::UniformDistributionType Base::createDistribution(const Regionfield& regionfield) {
	const Regionfield::ValueType region_count = regionfield.RegionCount;
	DRR_ASSERT(region_count > 0U);

	return UniformDistributionType(0U, region_count - 1U);
}