#include <DisRegRep/Container/BlendHistogram.hpp>
#include <DisRegRep/Maths/Arithmetic.hpp>

#include <algorithm>
#include <numeric>
#include <execution>

#include <cassert>

using std::array;
using std::ranges::copy, std::ranges::fill, std::ranges::for_each, std::equal,
	std::views::join, std::views::iota, std::views::transform, std::views::all,
	std::execution::unseq;

using namespace DisRegRep;
using Format::Region_t, Format::Bin_t, Format::NormBin_t, Format::SizeVec2, Format::SizeVec3;
using BlendHistogram::BasicDense, BlendHistogram::BasicSparseInternalStorage, BlendHistogram::BasicSparse;
using Arithmetic::horizontalProduct;

template<typename T>
bool BasicDense<T>::operator==(const BasicDense& comp) const {
	return equal(unseq, this->Histogram.cbegin(), this->Histogram.cend(), comp.Histogram.cbegin());
}

template<typename T>
size_t BasicDense<T>::sizeByte() const noexcept {
	return this->Histogram.size() * sizeof(value_type);
}

template<typename T>
void BasicDense<T>::resize(const SizeVec2& dimension, const Region_t region_count) {
	SizeVec3 dim_3;
	copy(dimension, dim_3.begin());
	dim_3.back() = region_count;

	this->Histogram.resize(horizontalProduct(dim_3));
	this->DenseIndexer = DenseIndexer_t(dim_3);
}

#define INS_DENSE(TYPE) template class BasicDense<TYPE>
INS_DENSE(Bin_t);
INS_DENSE(NormBin_t);

template<typename T>
size_t BasicSparseInternalStorage<T>::sizeByte() const noexcept {
	const auto bin_row = this->Bin | all | transform(
		[](const auto& bin) constexpr noexcept { return bin.size(); });
	return this->Offset.size() * sizeof(offset_type)
		+ this->Bin.size() * sizeof(typename decltype(this->Bin)::value_type)
		+ std::reduce(bin_row.cbegin(), bin_row.cend()) * sizeof(bin_type);
}

template<typename T>
void BasicSparseInternalStorage<T>::resize(SizeVec2 dimension, Region_t) {
	//we keep one extra offset at the end each x-axis to indicate the total number of bins.
	auto& [x, y] = dimension;
	x++;

	this->Offset.resize(horizontalProduct(dimension));
	this->Bin.resize(y);
	this->OffsetIndexer = BinOffsetIndexer_t(dimension);

	this->clear();
}

template<typename T>
void BasicSparseInternalStorage<T>::clear() {
	//initialise starting offset for each row in the new histogram
	fill(this->Offset, offset_type { });
	//we will be pushing new values into sparse bins, so need to clear the old ones
	for_each(this->Bin, [](auto& bin_y) constexpr noexcept { bin_y.clear(); });
}

#define INS_SPARSE_INTERNAL(TYPE) template class BasicSparseInternalStorage<TYPE>
INS_SPARSE_INTERNAL(Bin_t);
INS_SPARSE_INTERNAL(NormBin_t);

template<typename T>
void BasicSparse<T, true>::sortStorage() {
	const auto [dim_x, dim_y] = this->OffsetIndexer.extent();
	for (const auto y : iota(size_t { 0 }, dim_y)) {
		const auto row_it = this->Bin[y].begin();

		for (const auto x : iota(size_t { 0 }, dim_x - 1u)) {
			const auto [first, last] = this->getBinBound(x, y);
			std::ranges::sort(row_it + first, row_it + last, { },
				[](const auto& bin) constexpr noexcept { return bin.Region; });
		}
	}
}

template<typename T>
BasicSparse<T, true>& BasicSparse<T, true>::operator=(BasicSparse<T, false>&& unsorted) {
	using std::move;
	this->Offset = move(unsorted.Offset);
	this->Bin = move(unsorted.Bin);
	this->OffsetIndexer = move(unsorted.OffsetIndexer);

	this->sortStorage();
	return *this;
}

template<typename T>
bool BasicSparse<T, true>::operator==(const BasicSparse& comp) const {
	const auto left_bin = this->Bin | join,
		right_bin = comp.Bin | join;
	return equal(unseq, this->Offset.cbegin(), this->Offset.cend(), comp.Offset.cbegin())
		&& equal(unseq, left_bin.cbegin(), left_bin.cend(), right_bin.cbegin());
}

#define INS_SPARSE(TYPE) template class BasicSparse<TYPE, true>; \
template class BasicSparse<TYPE, false>
INS_SPARSE(Bin_t);
INS_SPARSE(NormBin_t);

template<typename U>
bool BlendHistogram::operator==(const BasicDense<U>& a, const BasicSparse<U, true>& b) {
	//first compare their sizes
	const auto [dense_x, dense_y, dense_region_count] = a.DenseIndexer.extent();
	const auto [sparse_x, sparse_y] = b.OffsetIndexer.extent();
	if (dense_x != sparse_x - 1u || dense_y != sparse_y) {
		return false;
	}

	using Format::Region_t;
	for (const auto y : iota(size_t { 0 }, dense_y)) {
		for (const auto x : iota(size_t { 0 }, dense_x)) {
			const auto dense_hist = a(x, y);
			const auto sparse_hist = b(x, y);

			//It should be sorted against region for every histogram
			//	because of how we constructed the sparse histogram in first place (by enumerate).
			assert(std::ranges::is_sorted(sparse_hist, { },
				[](const auto& bin) constexpr noexcept { return bin.Region; }));
			const size_t expected_region_count = std::max<size_t>(sparse_hist.back().Region + 1u, dense_region_count);

			//Basically we are forging a dense histogram from sparse, and compare two dense histograms as usual.
			auto sparse_forged_dense = iota(Region_t { 0 }, expected_region_count)
				| transform([sparse_it = sparse_hist.cbegin(), sparse_end = sparse_hist.cend()]
					(const auto dense_region) mutable constexpr noexcept {
					if (sparse_it == sparse_end) {
						return U { };
					}
					const auto [sparse_region, value] = *sparse_it;
					if (sparse_region == dense_region) {
						sparse_it++;
						return value;
					}
					return U { };
				});
			if (!std::ranges::equal(dense_hist, sparse_forged_dense)) {
				return false;
			}
		}
	}

	return true;
}

template<typename U>
bool BlendHistogram::operator==(const BasicSparse<U, true>& a, const BasicDense<U>& b) {
	return b == a;
}

#define INS_EQ(TYPE) \
template bool BlendHistogram::operator==<TYPE>(const BasicDense<TYPE>&, const BasicSparse<TYPE, true>&); \
template bool BlendHistogram::operator==<TYPE>(const BasicSparse<TYPE, true>&, const BasicDense<TYPE>&)
INS_EQ(Bin_t);
INS_EQ(NormBin_t);