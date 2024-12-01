#include <DisRegRep/Container/Regionfield.hpp>

#include <DisRegRep/Math/Arithmetic.hpp>
#include <DisRegRep/Type.hpp>

using namespace DisRegRep::Container;
using DisRegRep::Math::Arithmetic::horizontalProduct;
using DisRegRep::Type::SizeVec2;

void Regionfield::reshape(const SizeVec2 dim) {
	this->Data.resize(horizontalProduct(dim));
	this->View = MdSpanType(this->Data.data(), dim.x, dim.y);
}

SizeVec2 Regionfield::dimension() const noexcept {
	const auto& ext = this->View.extents();
	return { ext.extent(0U), ext.extent(1U) };
}