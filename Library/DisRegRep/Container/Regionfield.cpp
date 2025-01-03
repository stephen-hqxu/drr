#include <DisRegRep/Container/Regionfield.hpp>

#include <DisRegRep/Core/Arithmetic.hpp>
#include <DisRegRep/Core/Exception.hpp>

#include <glm/vector_relational.hpp>

#include <tuple>

#include <algorithm>
#include <execution>
#include <functional>
#include <ranges>

#include <cassert>

using DisRegRep::Container::Regionfield;

using std::apply, std::make_tuple;
using std::for_each, std::ranges::swap_ranges,
	std::execution::par_unseq,
	std::bind_front,
	std::views::zip, std::views::enumerate, std::views::transform, std::views::drop, std::views::take,
	std::ranges::range_const_reference_t;

void Regionfield::transpose() {
	const IndexType stride = this->mapping().stride(1U);
	const auto original = this->span() | Core::Arithmetic::View2d(stride);
	const auto transposed = this->span() | Core::Arithmetic::ViewTransposed2d(stride);
	const IndexType original_size = original.size();
	assert(original_size == transposed.size());

	//A simple, efficient, parallelisable, in-place matrix transpose method.
	const auto rg = zip(original, transposed)
		| enumerate
		| transform([](const auto it) static constexpr noexcept {
			const auto& [diag_idx, zip_tuple] = it;
			return apply([diag_idx](const auto&... row_column) constexpr noexcept {
				//Drop to ensure it does not overlap with elements before, otherwise it will cause data to race.
				//Exclude diagonal elements because it is stationary.
				return make_tuple(row_column | drop(diag_idx + 1)...);
			}, zip_tuple);
		})
		//A tiny optimisation to exclude the last range,
		//	which is essentially an empty range because it only contains a single diagonal element.
		| take(original_size - 1U);
	for_each(par_unseq, rg.cbegin(), rg.cend(),
		bind_front(apply<decltype((swap_ranges)), range_const_reference_t<decltype(rg)>>, swap_ranges));
}

void Regionfield::resize(const DimensionType dim) {
	DRR_ASSERT(glm::all(glm::greaterThan(dim, DimensionType(0U))));

	this->Mapping = ExtentType(dim.x, dim.y);
	this->Data.resize(this->Mapping.required_span_size());
}