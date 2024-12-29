#include <DisRegRep/Splatting/BaseFullConvolution.hpp>

#include <DisRegRep/Core/Exception.hpp>

#include <glm/vector_relational.hpp>

#include <utility>

#include <cstddef>

using DisRegRep::Splatting::BaseFullConvolution;

using glm::all, glm::greaterThan, glm::greaterThanEqual, glm::lessThanEqual;

using std::index_sequence, std::make_index_sequence;

void BaseFullConvolution::validate(const InvokeInfo& info) const {
	const auto [regionfield, offset, extent] = info;

	const auto rf_extent = [&ext = regionfield->mapping().extents()]<std::size_t... I>(index_sequence<I...>) constexpr noexcept {
		return DimensionType(ext.extent(I)...);
	}(make_index_sequence<DimensionType::length()> {});

	DRR_ASSERT(all(greaterThan(rf_extent, DimensionType(0U))));
	DRR_ASSERT(regionfield->RegionCount > 0U);
	DRR_ASSERT(all(greaterThan(extent, DimensionType(0U))));

	DRR_ASSERT(all(greaterThanEqual(offset, DimensionType(this->Radius))));
	DRR_ASSERT(all(lessThanEqual(offset + extent, rf_extent)));
}