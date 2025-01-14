#include <DisRegRep/Container/Regionfield.hpp>

#include <DisRegRep/Core/Arithmetic.hpp>
#include <DisRegRep/Core/Exception.hpp>

#include <glm/vector_relational.hpp>

#include <algorithm>
#include <execution>
#include <ranges>

#include <utility>

#include <cstddef>

using DisRegRep::Container::Regionfield;

using std::for_each, std::ranges::copy,
	std::execution::par_unseq,
	std::views::zip;
using std::index_sequence;

Regionfield Regionfield::transpose() const {
	//Make a fresh copy instead of just changing the stride (as a transposed view).
	//Although transpose is now much more expensive,
	//	it gives better cache locality when iterating through the matrix in other places.
	//It is possible to do an in-place transposition (by swapping upper and lower diagonal), but only with a square matrix.
	//It is also possible to use an in-place permute algorithm,
	//	but performance is generally poor on large matrix (even though it saves us memory) as not being parallelisable.
	Regionfield transposed;
	transposed.RegionCount = this->RegionCount;
	const auto transposed_dim = [&ext = this->mapping().extents()]<std::size_t... I>(index_sequence<I...>) constexpr noexcept {
		return DimensionType(ext.extent(I)...);
	}(index_sequence<1U, 0U> {});
	transposed.resize(transposed_dim);

	//Cannot use join on parallel algorithm because it is not a forward range.
	//Mainly because of the xvalue return on the 2D range adaptor, such that inner range is not a reference type.
	const auto zip_data = zip(
		this->Data | Core::Arithmetic::View2d(this->mapping().stride(1U)),
		transposed.Data | Core::Arithmetic::ViewTransposed2d(transposed.mapping().stride(1U))
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