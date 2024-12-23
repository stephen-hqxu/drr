#include <DisRegRep/Container/Splatting.hpp>
#include <DisRegRep/Container/SparseMatrixElement.hpp>

#include <DisRegRep/Exception.hpp>
#include <DisRegRep/Type.hpp>

#include <glm/vector_relational.hpp>

#include <span>
#include <tuple>

#include <algorithm>
#include <execution>
#include <functional>
#include <ranges>

namespace Splatting = DisRegRep::Container::Splatting;
using Splatting::BasicDense, Splatting::BasicSparse;
using DisRegRep::Type::RegionImportance, DisRegRep::Type::RegionMask;

using std::span,
	std::tie, std::apply;
using std::equal, std::for_each, std::all_of,
	std::ranges::fill,
	std::execution::par_unseq, std::execution::unseq,
	std::views::zip_transform;
using std::mem_fn, std::identity;

template<typename V>
bool Splatting::operator==(const BasicDense<V>& dense, const BasicSparse<V>& sparse) {
	DRR_ASSERT(sparse.isSorted());

	const auto compare_rg = zip_transform(
		[](const auto dense_proxy, const auto sparse_proxy) static constexpr {
			return std::ranges::equal(*dense_proxy | SparseMatrixElement::ToSparse, *sparse_proxy);
		},
		dense.range(), sparse.range());
	return all_of(par_unseq, compare_rg.cbegin(), compare_rg.cend(), identity {});
}

template<typename V>
bool Splatting::operator==(const BasicSparse<V>& sparse, const BasicDense<V>& dense) {
	return dense == sparse;
}

template<typename V>
bool BasicDense<V>::operator==(const BasicDense& dense) const {
	return equal(unseq, this->DenseMatrix.cbegin(), this->DenseMatrix.cend(), dense.DenseMatrix.cbegin(), dense.DenseMatrix.cend());
}

template<typename V>
typename BasicDense<V>::SizeType BasicDense<V>::sizeByte() const noexcept {
	return span(this->DenseMatrix).size_bytes();
}

template<typename V>
void BasicDense<V>::resize(const Dimension3Type dim) {
	DRR_ASSERT(glm::all(glm::greaterThan(dim, Dimension3Type(0U))));

	this->Mapping = ExtentType(dim.x, dim.y, dim.z);
	this->DenseMatrix.resize(this->Mapping.required_span_size());
}

template<typename V>
bool BasicSparse<V>::operator==(const BasicSparse& sparse) const {
	DRR_ASSERT(apply([](const auto&... matrix) static { return (matrix.isSorted() && ...); }, tie(*this, sparse)));

	return equal(unseq, this->Offset.cbegin(), this->Offset.cend(), sparse.Offset.cbegin(), sparse.Offset.cend())
		&& equal(
			unseq, this->SparseMatrix.cbegin(), this->SparseMatrix.cend(), sparse.SparseMatrix.cbegin(), sparse.SparseMatrix.cend());
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
	return span(this->Offset).size_bytes() + span(this->SparseMatrix).size_bytes();
}

template<typename V>
void BasicSparse<V>::resize(const Dimension3Type dim) {
	//Region count is unused in sparse matrix.
	const auto dim_xy = Type::Dimension2Type(dim.y, dim.z);
	DRR_ASSERT(glm::all(glm::greaterThan(dim_xy, Type::Dimension2Type(0U))));

	this->OffsetMapping = OffsetExtentType(dim_xy.x, dim_xy.y);
	this->Offset.resize(this->sizeOffset());
}

template<typename V>
void BasicSparse<V>::clear() {
	//Initialise starting offset for each element in the new sparse matrix.
	fill(this->Offset, OffsetType {});
	//We will be pushing new elements into the matrix, so need to clear the old ones.
	this->SparseMatrix.clear();
}

#define INSTANTIATE_DENSE(TYPE) template class DisRegRep::Container::Splatting::BasicDense<TYPE>
INSTANTIATE_DENSE(RegionImportance);
INSTANTIATE_DENSE(RegionMask);

#define INSTANTIATE_SPARSE(TYPE) template class DisRegRep::Container::Splatting::BasicSparse<TYPE>
INSTANTIATE_SPARSE(RegionImportance);
INSTANTIATE_SPARSE(RegionMask);

#define INSTANTIATE_EQ(TYPE) \
	template bool DisRegRep::Container::Splatting::operator==<TYPE>(const BasicDense<TYPE>&, const BasicSparse<TYPE>&); \
	template bool DisRegRep::Container::Splatting::operator==<TYPE>(const BasicSparse<TYPE>&, const BasicDense<TYPE>&)
INSTANTIATE_EQ(RegionImportance);
INSTANTIATE_EQ(RegionMask);