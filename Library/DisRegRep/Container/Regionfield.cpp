#include <DisRegRep/Container/Regionfield.hpp>

#include <DisRegRep/Core/View/Matrix.hpp>
#include <DisRegRep/Core/Exception.hpp>
#include <DisRegRep/Core/MdSpan.hpp>

#include <glm/vector_relational.hpp>

#include <algorithm>
#include <execution>
#include <ranges>

using DisRegRep::Container::Regionfield;

using std::for_each, std::ranges::copy,
	std::execution::par_unseq,
	std::views::zip;

Regionfield Regionfield::transpose() const {
	//Make a fresh copy instead of just changing the stride (as a transposed view).
	//Although transpose is now much more expensive,
	//	it gives better cache locality when iterating through the matrix in other places.
	//It is possible to do an in-place transposition (by swapping upper and lower diagonal), but only with a square matrix.
	//It is also possible to use an in-place permute algorithm,
	//	but performance is generally poor on large matrices (even though it saves us memory) as not being parallelisable.
	Regionfield transposed;
	transposed.RegionCount = this->RegionCount;
	transposed.resize(Core::MdSpan::reverse(this->extent()));

	//Cannot use join on a parallel algorithm because it is not a forward range.
	//Mainly because of the xvalue return on the 2D range adaptor, such that inner range is not a reference type.
	const auto zip_data = zip(
		this->Data | Core::View::Matrix::NewAxisLeft(this->Mapping.stride(0U)),
		transposed.Data | Core::View::Matrix::NewAxisRight(transposed.Mapping.stride(0U))
	);
	for_each(par_unseq, zip_data.cbegin(), zip_data.cend(), [](const auto it) static constexpr noexcept {
		const auto& [input, output] = it;
		copy(input, output.begin());
	});

	return transposed;
}

void Regionfield::resize(const DimensionType dim) {
	DRR_ASSERT(glm::all(glm::greaterThan(dim, DimensionType(0U))));

	this->Mapping = Core::MdSpan::toExtent(dim);
	this->Data.resize(this->Mapping.required_span_size());
}

Regionfield::DimensionType Regionfield::extent() const noexcept {
	return Core::MdSpan::toVector(this->Mapping.extents());
}