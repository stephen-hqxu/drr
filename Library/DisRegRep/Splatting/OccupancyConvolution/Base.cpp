#include <DisRegRep/Splatting/OccupancyConvolution/Base.hpp>
#include <DisRegRep/Splatting/Base.hpp>

#include <DisRegRep/Container/Regionfield.hpp>

using DisRegRep::Splatting::OccupancyConvolution::Base,
	DisRegRep::Container::Regionfield;

Base::DimensionType Base::minimumRegionfieldDimension(const InvokeInfo& invoke_info) const noexcept {
	return this->Splatting::Base::minimumRegionfieldDimension(invoke_info) + this->Radius;
}

Base::DimensionType Base::minimumOffset() const noexcept {
	return this->Splatting::Base::minimumOffset() + this->Radius;
}

Base::DimensionType Base::maximumExtent(const Regionfield& regionfield, const DimensionType offset) const noexcept {
	return this->Splatting::Base::maximumExtent(regionfield, offset) - this->Radius;
}