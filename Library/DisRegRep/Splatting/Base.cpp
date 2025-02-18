#include <DisRegRep/Splatting/Base.hpp>

#include <DisRegRep/Container/Regionfield.hpp>
#include <DisRegRep/Core/Exception.hpp>

#include <glm/vector_relational.hpp>

using DisRegRep::Splatting::Base;

using glm::all, glm::greaterThanEqual;

void Base::validate(const DisRegRep::Container::Regionfield& regionfield, const InvokeInfo& info) const {
	const auto [offset, extent] = info;
	const auto rf_extent = regionfield.extent();

	DRR_ASSERT(regionfield.RegionCount > 0U);
	DRR_ASSERT(all(greaterThanEqual(rf_extent, this->minimumRegionfieldDimension(info))));
	DRR_ASSERT(all(greaterThanEqual(offset, this->minimumOffset())));
}

Base::DimensionType Base::minimumRegionfieldDimension(const InvokeInfo& info) const noexcept {
	const auto [offset, extent] = info;
	return offset + extent;
}

Base::DimensionType Base::minimumOffset() const noexcept {
	return DimensionType(0U);
}