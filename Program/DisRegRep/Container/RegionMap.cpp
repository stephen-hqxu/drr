#include <DisRegRep/Container/RegionMap.hpp>
#include <DisRegRep/Maths/Arithmetic.hpp>

#include <cassert>

using namespace DisRegRep;
using Format::SizeVec2;

size_t RegionMap::reshape(const SizeVec2& dimension) {
	const size_t new_size = Arithmetic::horizontalProduct(dimension);

	this->Map.resize(new_size);
	this->Dimension = dimension;
	this->RegionMapIndexer = RegionMapIndexer_t(dimension);
	return new_size;
}

size_t RegionMap::size() const noexcept {
	const size_t s = this->Map.size();
	assert(s == Arithmetic::horizontalProduct(this->Dimension));
	return s;
}