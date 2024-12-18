#include <DisRegRep/Container/Regionfield.hpp>

#include <DisRegRep/Exception.hpp>

#include <glm/vector_relational.hpp>

using DisRegRep::Container::Regionfield;

void Regionfield::resize(const DimensionType dim) {
	DRR_ASSERT(glm::all(glm::greaterThan(dim, DimensionType(0U))));

	this->Mapping = ExtentType(dim.x, dim.y);
	this->Data.resize(this->Mapping.required_span_size());
}