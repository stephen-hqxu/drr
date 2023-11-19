#include <DisRegRep/Container/SingleHistogram.hpp>
#include <DisRegRep/Maths/Arithmetic.hpp>

#include <execution>
#include <algorithm>

using std::array;
using std::ranges::copy, std::ranges::fill, std::ranges::for_each, std::equal,
	std::views::stride, std::views::join,
	std::execution::unseq;

using namespace DisRegRep;
using Format::Bin_t, Format::NormBin_t, Format::SizeVec2;
using SingleHistogram::BasicDense, SingleHistogram::BasicSparse;
using Arithmetic::horizontalProduct;

template<typename TBin>
bool BasicDense<TBin>::operator==(const BasicDense& comp) const {
	return equal(unseq, this->Histogram.cbegin(), this->Histogram.cend(), comp.Histogram.cbegin());
}

template<typename TBin>
void BasicDense<TBin>::reshape(const SizeVec2& dimension, const size_t bin_count) {
	array<size_t, 3u> dim_3;
	copy(dimension, dim_3.begin());
	dim_3.back() = bin_count;

	this->Histogram.resize(horizontalProduct(dim_3));
	this->DenseIndexer = DenseIndexer_t(dim_3);
}

template DRR_API class BasicDense<Bin_t>;
template DRR_API class BasicDense<NormBin_t>;

template<typename TBin>
bool BasicSparse<TBin>::operator==(const BasicSparse& comp) const {
	const auto left_bin = this->Bin | join,
		right_bin = comp.Bin | join;
	return equal(unseq, this->Offset.cbegin(), this->Offset.cend(), comp.Offset.cbegin())
		&& equal(unseq, left_bin.cbegin(), left_bin.cend(), right_bin.cbegin());
}

template<typename TBin>
void BasicSparse<TBin>::reshape(SizeVec2 dimension) {
	//we keep one extra offset at the end each x-axis to indicate the total number of bins.
	auto& [x, y] = dimension;
	x++;

	this->Offset.resize(horizontalProduct(dimension));
	this->Bin.resize(y);
	this->OffsetIndexer = BinOffsetIndexer_t(x, y);

	//initialise starting offset for each row in the new histogram
	fill(this->Offset | stride(x), BinOffset_t { 0 });
	for_each(this->Bin, [](auto& bin_y) constexpr noexcept { bin_y.clear(); });
}

template DRR_API class BasicSparse<Bin_t>;
template DRR_API class BasicSparse<NormBin_t>;