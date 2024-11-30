#include <DisRegRep/Container/RegionMap.hpp>
#include <DisRegRep/Maths/Arithmetic.hpp>

#include <cassert>

using namespace DisRegRep;
using Format::SizeVec2;

void RegionMap::reshape(const SizeVec2& dimension) {
	this->Map.resize(Arithmetic::horizontalProduct(dimension));
	this->Dimension = dimension;
	this->RegionMapIndexer = RegionMapIndexer_t(dimension);
}

size_t RegionMap::size() const noexcept {
	const size_t s = this->Map.size();
	assert(s == Arithmetic::horizontalProduct(this->Dimension));
	return s;
}