#include <DisRegRep/Splatting/Base.hpp>

#include <DisRegRep/Container/Regionfield.hpp>
#include <DisRegRep/Core/Exception.hpp>
#include <DisRegRep/Core/XXHash.hpp>

#include <glm/vector_relational.hpp>

namespace XXHash = DisRegRep::Core::XXHash;
using DisRegRep::Splatting::Base,
	DisRegRep::Container::Regionfield;

using glm::all, glm::greaterThanEqual, glm::lessThanEqual;

void Base::validate(const InvokeInfo& invoke_info, const Regionfield& regionfield) const {
	const auto [offset, extent] = invoke_info;
	const auto rf_extent = regionfield.extent();

	DRR_ASSERT(regionfield.RegionCount > 0U);
	DRR_ASSERT(all(greaterThanEqual(rf_extent, this->minimumRegionfieldDimension(invoke_info))));
	DRR_ASSERT(all(greaterThanEqual(offset, this->minimumOffset())));
	DRR_ASSERT(all(lessThanEqual(extent, this->maximumExtent(regionfield, offset))));
}

XXHash::Secret Base::generateSecret(const SeedType seed) {
	return XXHash::generateSecret(
		XXHash::makeApplicationSecret("d6 f2 1f 34 e7 a8 9b df 2c f7 bd 1c 05 75 10 32 a7 98 37 2e eb 9b 15 64 94 3e 0a cb 6e f4 d3 3c 91 84 82 ee be 85 ff cc 4e 86 6c 89 b8 42 c6 1f 59 16 42 36 3b eb ad 03 82 29 ca fa 79 d6 ae a3 f6 f0 e2 97 ff be 4e 1f a1 8d 99 62 3f 3d 75 ad"),
		seed
	);
}

Base::DimensionType Base::minimumRegionfieldDimension(const InvokeInfo& invoke_info) const noexcept {
	const auto [offset, extent] = invoke_info;
	return offset + extent;
}

Base::DimensionType Base::minimumOffset() const noexcept {
	return DimensionType(0U);
}

Base::DimensionType Base::maximumExtent(const Regionfield& regionfield, const DimensionType offset) const noexcept {
	return regionfield.extent() - offset;
}