#include <DisRegRep/Factory/RegionMapFactory.hpp>

using namespace DisRegRep;
using namespace Format;

RegionMap RegionMapFactory::allocateRegionMap(const SizeVec2& dimension) const {
	const auto [x, y] = dimension;
	return {
		.Map = x * y,
		.Dimension = dimension,
	};
}

RegionMap RegionMapFactory::operator()(const CreateDescription& desc, const SizeVec2& dimension) const {
	RegionMap map = this->allocateRegionMap(dimension);
	(*this)(desc, map);
	return map;
}