#include <DisRegRep/Splatting/OccupancyConvolution/Base.hpp>
#include <DisRegRep/Splatting/Base.hpp>

#include <DisRegRep/Container/Regionfield.hpp>

#include <DisRegRep/Core/Exception.hpp>

#include <glm/vector_relational.hpp>

using DisRegRep::Splatting::OccupancyConvolution::Base,
	DisRegRep::Container::Regionfield;

Base::DimensionType Base::minimumRegionfieldDimension(const InvokeInfo& invoke_info) const {
	return this->Splatting::Base::minimumRegionfieldDimension(invoke_info) + this->Radius;
}

Base::DimensionType Base::minimumOffset() const {
	return this->Splatting::Base::minimumOffset() + this->Radius;
}

Base::DimensionType Base::maximumExtent(const Regionfield& regionfield, const DimensionType offset) const {
	const DimensionType base_max_extent = this->Splatting::Base::maximumExtent(regionfield, offset);
	DRR_ASSERT(glm::all(glm::greaterThanEqual(base_max_extent, DimensionType(this->Radius))));
	return base_max_extent - this->Radius;
}