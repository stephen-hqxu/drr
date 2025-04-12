#include <DisRegRep/RegionfieldGenerator/Base.hpp>

#include <DisRegRep/Container/Regionfield.hpp>

#include <DisRegRep/Core/Exception.hpp>
#include <DisRegRep/Core/XXHash.hpp>

namespace XXHash = DisRegRep::Core::XXHash;
using DisRegRep::RegionfieldGenerator::Base;

XXHash::Secret Base::generateSecret(const GenerateInfo& gen_info) {
	const auto [seed] = gen_info;
	return XXHash::generateSecret(
		XXHash::makeApplicationSecret("60 e6 5a 64 a2 20 db 7d 46 b5 f3 db ba 03 7f e2 38 75 3d 57 a3 45 d2 f7 f5 d2 2c 31 48 05 00 4a 6a 72 b6 c2 24 ad c0 e8 39 ae de 6f a4 56 08 25 52 b6 52 22 3c 4d b1 c6 2a b0 c9 a4 25 3d 38 21 13 1a 05 ac 68 62 c4 cd 12 00 e2 c4 cd 92 b8 be"),
		seed
	);
}

Base::UniformDistributionType Base::createDistribution(const Container::Regionfield& regionfield) {
	const Container::Regionfield::ValueType region_count = regionfield.RegionCount;
	DRR_ASSERT(region_count > 0U);

	return UniformDistributionType(0U, region_count - 1U);
}