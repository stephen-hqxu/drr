#include <DisRegRep/Container/SplattingCoefficient.hpp>

#include <DisRegRep/Core/Exception.hpp>
#include <DisRegRep/Core/MdSpan.hpp>
#include <DisRegRep/Core/Type.hpp>

#include <glm/vector_relational.hpp>

#include <span>
#include <tuple>

#include <algorithm>
#include <execution>
#include <functional>

namespace SpltCoef = DisRegRep::Container::SplattingCoefficient;
using SpltCoef::BasicDense, SpltCoef::BasicSparse;
using DisRegRep::Core::Type::RegionImportance, DisRegRep::Core::Type::RegionMask;

using std::span,
	std::tie, std::apply;
using std::for_each, std::all_of,
	std::execution::par_unseq;
using std::mem_fn;

template<typename V>
typename BasicDense<V>::SizeType BasicDense<V>::sizeByte() const noexcept {
	return span(this->DenseMatrix).size_bytes();
}

template<typename V>
void BasicDense<V>::resize(const Dimension3Type dim) {
	DRR_ASSERT(glm::all(glm::greaterThan(dim, Dimension3Type(0U))));

	this->Mapping = Core::MdSpan::toExtent(dim);
	this->DenseMatrix.resize(this->Mapping.required_span_size());
}

template<typename V>
void BasicSparse<V>::sort() {
	using std::ranges::sort;
	const auto rg = this->range();
	for_each(par_unseq, rg.begin(), rg.end(),
		[](const auto proxy) static constexpr { sort(*proxy, {}, mem_fn(&ElementType::Identifier)); });
}

template<typename V>
bool BasicSparse<V>::isSorted() const {
	using std::ranges::is_sorted;
	const auto rg = this->range();
	return all_of(par_unseq, rg.cbegin(), rg.cend(),
		[](const auto proxy) static constexpr { return is_sorted(*proxy, {}, mem_fn(&ElementType::Identifier)); });
}

template<typename V>
typename BasicSparse<V>::SizeType BasicSparse<V>::sizeByte() const noexcept {
	return apply([](const auto&... matrix) static constexpr noexcept { return (span(matrix).size_bytes() + ...); },
		tie(this->Offset, this->SparseMatrix));
}

template<typename V>
void BasicSparse<V>::resize(const Dimension3Type dim) {
	//Region count is unused in sparse matrix.
	const Type::Dimension2Type dim_wh = dim;
	DRR_ASSERT(glm::all(glm::greaterThan(dim_wh, Type::Dimension2Type(0U))));

	this->OffsetMapping = Core::MdSpan::toExtent(dim_wh);
	//Initialise starting offset for each element in the new sparse matrix.
	this->Offset.resize(this->sizeOffset());
	//We will be pushing new elements into the matrix, so need to clear the old ones.
	this->SparseMatrix.clear();
}

#define INSTANTIATE_DENSE(TYPE) template class DisRegRep::Container::SplattingCoefficient::BasicDense<TYPE>
#define INSTANTIATE_SPARSE(TYPE) template class DisRegRep::Container::SplattingCoefficient::BasicSparse<TYPE>

#define INSTANTIATE_ALL(TYPE) \
	INSTANTIATE_DENSE(TYPE); \
	INSTANTIATE_SPARSE(TYPE)
INSTANTIATE_ALL(RegionImportance);
INSTANTIATE_ALL(RegionMask);