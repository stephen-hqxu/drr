#include <DisRegRep/Splatting/Base.hpp>

#include <DisRegRep/Core/Exception.hpp>

#include <glm/vector_relational.hpp>

#include <utility>

#include <cstddef>

using DisRegRep::Splatting::Base;

using glm::all, glm::greaterThanEqual;

using std::index_sequence, std::make_index_sequence;

void Base::validate(const InvokeInfo& info) const {
	const auto [regionfield, offset, extent] = info;

	const auto rf_extent = [&ext = regionfield->mapping().extents()]<std::size_t... I>(index_sequence<I...>) constexpr noexcept {
		return DimensionType(ext.extent(I)...);
	}(make_index_sequence<DimensionType::length()> {});

	DRR_ASSERT(regionfield->RegionCount > 0U);
	DRR_ASSERT(all(greaterThanEqual(rf_extent, this->minimumRegionfieldDimension(info))));
	DRR_ASSERT(all(greaterThanEqual(offset, this->minimumOffset(info))));
}

Base::DimensionType Base::minimumRegionfieldDimension(const InvokeInfo& info) const noexcept {
	const auto [_, offset, extent] = info;
	return offset + extent;
}

Base::DimensionType Base::minimumOffset(const InvokeInfo&) const noexcept {
	return DimensionType(0U);
}