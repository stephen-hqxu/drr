#include <DisRegRep/Splatting/Convolution/Base.hpp>
#include <DisRegRep/Splatting/Base.hpp>

using DisRegRep::Splatting::Convolution::Base;

Base::DimensionType Base::minimumRegionfieldDimension(const BaseInvokeInfo& info) const noexcept {
	return this->Splatting::Base::minimumRegionfieldDimension(info) + dynamic_cast<const InvokeInfo&>(info).Radius;
}

Base::DimensionType Base::minimumOffset(const BaseInvokeInfo& info) const noexcept {
	return this->Splatting::Base::minimumOffset(info) + dynamic_cast<const InvokeInfo&>(info).Radius;
}