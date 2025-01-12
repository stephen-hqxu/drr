#include <DisRegRep/Splatting/BaseFullConvolution.hpp>
#include <DisRegRep/Splatting/Base.hpp>

using DisRegRep::Splatting::BaseFullConvolution;

BaseFullConvolution::DimensionType BaseFullConvolution::minimumRegionfieldDimension(const InvokeInfo& info) const noexcept {
	return this->Base::minimumRegionfieldDimension(info) + this->Radius;
}

BaseFullConvolution::DimensionType BaseFullConvolution::minimumOffset(const InvokeInfo&) const noexcept {
	return DimensionType(this->Radius);
}