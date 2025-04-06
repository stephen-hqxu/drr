#include <DisRegRep/Splatting/Convolution/Base.hpp>
#include <DisRegRep/Splatting/Base.hpp>

#include <DisRegRep/Container/Regionfield.hpp>

using DisRegRep::Splatting::Convolution::Base,
	DisRegRep::Container::Regionfield;

Base::DimensionType Base::minimumRegionfieldDimension(const InvokeInfo& info) const noexcept {
	return this->Splatting::Base::minimumRegionfieldDimension(info) + this->Radius;
}

Base::DimensionType Base::minimumOffset() const noexcept {
	return this->Splatting::Base::minimumOffset() + this->Radius;
}

Base::DimensionType Base::maximumExtent(const Regionfield& regionfield, const DimensionType offset) const noexcept {
	return this->Splatting::Base::maximumExtent(regionfield, offset) - this->Radius;
}