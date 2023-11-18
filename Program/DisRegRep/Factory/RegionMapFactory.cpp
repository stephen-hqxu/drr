#include <DisRegRep/Factory/RegionMapFactory.hpp>

using namespace DisRegRep;
using namespace Format;

using Arithmetic::horizontalProduct;

RegionMap RegionMapFactory::allocate(const SizeVec2& dimension) {
	RegionMap map;
	map.resizeToDimension(dimension);
	return map;
}

bool RegionMapFactory::reshape(RegionMap& region_map, const SizeVec2& dimension) {
	const size_t old_size = region_map.size(),
		new_size = region_map.resizeToDimension(dimension);

	//determine if original contents are undefined
	if (new_size > old_size) {
		return true;
	} else {
		return false;
	}
}

RegionMap RegionMapFactory::operator()(const CreateDescription& desc, const SizeVec2& dimension) const {
	RegionMap map = RegionMapFactory::allocate(dimension);
	(*this)(desc, map);
	return map;
}