#include <DisRegRep/Container/Regionfield.hpp>

#include <DisRegRep/Exception.hpp>

#include <glm/vec2.hpp>
#include <glm/vector_relational.hpp>

#include <memory>

using namespace DisRegRep::Container;

using glm::vec;

using std::make_unique_for_overwrite;

Regionfield::Regionfield(const DimensionType dim, const ValueType region_count) :
	Mapping(ExtentType(dim.x, dim.y)),
	Data(make_unique_for_overwrite<ValueType[]>(this->Mapping.required_span_size())),
	RegionCount(region_count) {
	DRR_ASSERT(glm::all(glm::greaterThan(dim, DimensionType(0U))));
	DRR_ASSERT(region_count > 0U);
}