#include <DisRegRep/Splatting/Convolution/Base.hpp>
#include <DisRegRep/Splatting/Base.hpp>

using DisRegRep::Splatting::Convolution::Base;

Base::DimensionType Base::minimumRegionfieldDimension(const InvokeInfo& info) const noexcept {
	return this->Splatting::Base::minimumRegionfieldDimension(info) + this->Radius;
}

Base::DimensionType Base::minimumOffset(const InvokeInfo&) const noexcept {
	return DimensionType(this->Radius);
}