#include <DisRegRep/Splatting/Base.hpp>

#include <DisRegRep/Container/Regionfield.hpp>
#include <DisRegRep/Core/Exception.hpp>

#include <glm/vector_relational.hpp>

using DisRegRep::Splatting::Base,
	DisRegRep::Container::Regionfield;

using glm::all, glm::greaterThanEqual, glm::lessThanEqual;

void Base::validate(const Regionfield& regionfield, const InvokeInfo& invoke_info) const {
	const auto [offset, extent] = invoke_info;
	const auto rf_extent = regionfield.extent();

	DRR_ASSERT(regionfield.RegionCount > 0U);
	DRR_ASSERT(all(greaterThanEqual(rf_extent, this->minimumRegionfieldDimension(invoke_info))));
	DRR_ASSERT(all(greaterThanEqual(offset, this->minimumOffset())));
	DRR_ASSERT(all(lessThanEqual(extent, this->maximumExtent(regionfield, offset))));
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