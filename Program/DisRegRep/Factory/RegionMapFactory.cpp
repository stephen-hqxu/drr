#include <DisRegRep/Factory/RegionMapFactory.hpp>
#include <DisRegRep/Maths/Arithmetic.hpp>

#include <utility>

using namespace DisRegRep;
using namespace Format;

using Arithmetic::horizontalProduct;

RegionMap RegionMapFactory::allocate(const SizeVec2& dimension) {
	return {
		.Map = std::vector<Region_t>(horizontalProduct(dimension)),
		.Dimension = dimension,
	};
}

bool RegionMapFactory::reshape(RegionMap& region_map, const SizeVec2& dimension) {
	const size_t old_size = region_map.Map.size(),
		new_size = horizontalProduct(dimension);
	
	region_map.Map.resize(new_size);
	region_map.Dimension = dimension;
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