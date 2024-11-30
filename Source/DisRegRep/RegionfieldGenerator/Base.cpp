#include <DisRegRep/Factory/RegionMapFactory.hpp>

using namespace DisRegRep;
using Format::SizeVec2;

RegionMap RegionMapFactory::allocate(const SizeVec2& dimension) {
	RegionMap map;
	map.reshape(dimension);
	return map;
}

void RegionMapFactory::reshape(RegionMap& region_map, const SizeVec2& dimension) {
	region_map.reshape(dimension);
}

RegionMap RegionMapFactory::operator()(const CreateDescription& desc, const SizeVec2& dimension) const {
	RegionMap map = RegionMapFactory::allocate(dimension);
	(*this)(desc, map);
	return map;
}