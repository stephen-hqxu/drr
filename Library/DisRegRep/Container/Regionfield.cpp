#include <DisRegRep/Container/Regionfield.hpp>

#include <DisRegRep/Core/View/Matrix.hpp>
#include <DisRegRep/Core/Exception.hpp>

#include <glm/gtc/type_ptr.hpp>
#include <glm/vector_relational.hpp>

#include <span>

#include <algorithm>
#include <execution>
#include <ranges>

#include <utility>

#include <cstddef>

using DisRegRep::Container::Regionfield;

using glm::value_ptr;

using std::span;
using std::for_each, std::ranges::reverse, std::ranges::copy,
	std::execution::par_unseq,
	std::views::zip;
using std::index_sequence, std::make_index_sequence;

Regionfield Regionfield::transpose() const {
	//Make a fresh copy instead of just changing the stride (as a transposed view).
	//Although transpose is now much more expensive,
	//	it gives better cache locality when iterating through the matrix in other places.
	//It is possible to do an in-place transposition (by swapping upper and lower diagonal), but only with a square matrix.
	//It is also possible to use an in-place permute algorithm,
	//	but performance is generally poor on large matrix (even though it saves us memory) as not being parallelisable.
	Regionfield transposed;
	transposed.RegionCount = this->RegionCount;
	DimensionType transposed_extent = this->extent();
	reverse(::span(value_ptr(transposed_extent), DimensionType::length()));
	transposed.resize(transposed_extent);

	//Cannot use join on parallel algorithm because it is not a forward range.
	//Mainly because of the xvalue return on the 2D range adaptor, such that inner range is not a reference type.
	const auto zip_data = zip(
		this->Data | Core::View::Matrix::View2d(this->mapping().stride(1U)),
		transposed.Data | Core::View::Matrix::ViewTransposed2d(transposed.mapping().stride(1U))
	);
	for_each(par_unseq, zip_data.cbegin(), zip_data.cend(), [](const auto it) static constexpr noexcept {
		const auto& [input, output] = it;
		copy(input, output.begin());
	});

	return transposed;
}

void Regionfield::resize(const DimensionType dim) {
	DRR_ASSERT(glm::all(glm::greaterThan(dim, DimensionType(0U))));

	this->Mapping = ExtentType(dim.x, dim.y);
	this->Data.resize(this->Mapping.required_span_size());
}

Regionfield::DimensionType Regionfield::extent() const noexcept {
	return [&ext = this->mapping().extents()]<std::size_t... I>(index_sequence<I...>) constexpr noexcept {
		return DimensionType(ext.extent(I)...);
	}(make_index_sequence<ExtentType::rank()> {});
}