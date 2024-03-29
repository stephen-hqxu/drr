#pragma once

#include "ContainerTrait.hpp"
#include "SparseBin.hpp"

#include "../Format.hpp"
#include "../Maths/Arithmetic.hpp"
#include "../Maths/Indexer.hpp"

#include <vector>
#include <span>
#include <utility>
#include <ranges>

#include <cstdint>

namespace DisRegRep {

/**
 * @brief A single histogram is a matrix where each element points to a histogram, which is an array of bins.
*/
namespace SingleHistogram {

/**
 * @brief A dense single histogram uses a 3D matrix.
 *
 * @tparam T The bin type.
*/
template<typename>
class BasicDense;
/**
 * @brief A sparse single histogram is a 2D matrix of arrays with, useful if the bin axis is sparse.
 *
 * @tparam T The bin type.
 * @tparam Sorted Indicates if each histogram is sorted.
*/
template<typename, bool>
class BasicSparse;

template<typename U>
bool operator==(const BasicDense<U>&, const BasicSparse<U, true>&);
template<typename U>
bool operator==(const BasicSparse<U, true>&, const BasicDense<U>&);

#define FRIEND_DENSE_EQ_SPARSE \
template<typename U> \
friend bool operator==(const BasicDense<U>&, const BasicSparse<U, true>&); \
template<typename U> \
friend bool operator==(const BasicSparse<U, true>&, const BasicDense<U>&)

template<typename T>
class BasicDense {
public:

	using value_type = T;

private:

	std::vector<value_type> Histogram;

	using DenseIndexer_t = Indexer<1, 2, 0>;/**< Default indexing order of histogram. */
	DenseIndexer_t DenseIndexer;

	constexpr size_t getOffset(const auto x, const auto y) const noexcept {
		return this->DenseIndexer(x, y, 0);
	}

	//Get the iterator to the histogram given 2D coordinate.
	constexpr auto getHistogramIterator(const auto x, const auto y) noexcept {
		return this->Histogram.begin() + this->getOffset(x, y);
	}
	constexpr auto getHistogramIterator(const auto x, const auto y) const noexcept {
		return this->Histogram.cbegin() + this->getOffset(x, y);
	}

public:

	using BinView = std::span<value_type>;
	using ConstBinView = std::span<const value_type>;

	constexpr BasicDense() noexcept = default;

	BasicDense(const BasicDense&) = delete;

	constexpr BasicDense(BasicDense&&) noexcept = default;

	constexpr ~BasicDense() = default;

	/**
	 * @brief Compare equality of two dense histograms.
	 * 
	 * @param comp The other dense histogram.
	 * 
	 * @return True if two histograms are identical.
	*/
	bool operator==(const BasicDense&) const;

	FRIEND_DENSE_EQ_SPARSE;

	/**
	 * @brief Get the number of byte the dense histogram occupies.
	 * 
	 * @return The size in byte.
	*/
	size_t sizeByte() const noexcept;

	/**
	 * @brief Resize the dimension of histogram.
	 * If dimension is shrunken, extra data will be trimmed.
	 * 
	 * @param dimension The new dimension.
	 * @param region_count The number of region per histogram.
	*/
	void resize(const Format::SizeVec2&, Format::Region_t);

	/**
	 * @brief This function must be called to reset internal state before commencing new computations.
	 * It's currently a no-op, but I am keeping it for consistency reasons.
	 * 
	 * @see BasicSparse::clear()
	*/
	constexpr void clear() const noexcept { }

	/**
	 * @brief Set bins of a histogram from a range.
	 * 
	 * @tparam R The range type.
	 * @tparam T The type of factor.
	 * 
	 * @param input The input range of bins to be filled into this histogram.
	 * @param x, y The coordinates into this histogram.
	 * @param factor Can be specified to scale the range before writing to the histogram.
	*/
	template<ContainerTrait::ValueRange<value_type> R>
	constexpr void operator()(R&& input, const auto x, const auto y) {
		std::ranges::copy(input, this->getHistogramIterator(x, y));
	}
	template<std::ranges::input_range R, typename T>
	requires(std::floating_point<value_type>)
	void operator()(R&& input, const auto x, const auto y, const T factor) {
		Arithmetic::scaleRange(std::forward<R>(input), this->getHistogramIterator(x, y), factor);
	}

	/**
	 * @brief Get the view into a histogram.
	 * 
	 * @param x, y The coordinates.
	 * 
	 * @return The view of all bins in a histogram.
	*/
	constexpr ConstBinView operator()(const auto x, const auto y) const noexcept {
		return ConstBinView(this->getHistogramIterator(x, y), this->DenseIndexer.extent(2u));
	}

};
using Dense = BasicDense<Format::Bin_t>;
using DenseNorm = BasicDense<Format::NormBin_t>;

#define SCALE_TRANSFORM static_cast<value_type>(value / factor)

/**
 * @brief This class provides storage, and should not be used directly.
 * 
 * @tparam T The bin type.
*/
template<typename T>
class BasicSparseInternalStorage {
public:

	using value_type = T;

protected:

	using bin_type = SparseBin::BasicSparseBin<value_type>;
	using offset_type = std::uint32_t;

	std::vector<offset_type> Offset;
	//List-of-List sparse matrix format, this is better than List-of-Flattened-List
	//	for allowing constructing the matrix in row-major or column-major order at the same time.
	std::vector<std::vector<bin_type>> Bin;

	using BinOffsetIndexer_t = Indexer<0, 1>;
	BinOffsetIndexer_t OffsetIndexer;

	//Without any checking.
	template<typename R>
	constexpr void insertBin(R&& input, const auto x, const auto y) {
		auto& bin_y = this->Bin[y];
		bin_y.append_range(input);
		this->Offset[this->OffsetIndexer(x + 1, y)] = static_cast<offset_type>(bin_y.size());
	}

	//Get the index bound of bins for a histogram.
	constexpr auto getBinBound(const auto x, const auto y) const noexcept {
		//as offset matrix is row-major linear, we are doing this to avoid recalculating the offset linear index twice
		const size_t offset_idx = this->OffsetIndexer(x, y),
			bin_start_idx = this->Offset[offset_idx],
			//this is equivalent to `this->OffsetIndexer(x + 1, y)`
			bin_end_idx = this->Offset[offset_idx + 1u];
		return std::make_pair(bin_start_idx, bin_end_idx);
	}

public:

	using ConstBinView = std::span<const bin_type>;

	constexpr BasicSparseInternalStorage() noexcept = default;

	BasicSparseInternalStorage(const BasicSparseInternalStorage&) = delete;

	constexpr BasicSparseInternalStorage(BasicSparseInternalStorage&&) noexcept = default;

	constexpr ~BasicSparseInternalStorage() = default;

	/**
	 * @brief Get the size in byte of this sparse histogram.
	 * 
	 * @return The size in byte.
	*/
	size_t sizeByte() const noexcept;

	/**
	 * @brief Resize the histogram.
	 * All contents in the histogram will be cleared.
	 *
	 * @param dimension The new dimension for the histogram.
	 * @param region_count The number of region per histogram.
	 * This argument is unused in sparse histogram, keeping this for API consistency.
	*/
	void resize(Format::SizeVec2, Format::Region_t);

	/**
	 * @brief Clear the sparse histogram for new computations.
	 * This function must be called to reset internal state of the histogram
	 * and clear old values before commencing any computations.
	*/
	void clear();

	/**
	 * @brief Push some bins from a dense histogram into the current sparse histogram.
	 * 
	 * @tparam R The type of input range.
	 * @tparam T The type of factor.
	 * 
	 * @param input The input range containing bins to be pushed.
	 * @param x, y The index to the histogram whose bins are belonging to.
	 * The behaviour is undefined if bins are not pushed contiguously in x axis.
	 * @param factor Provide a factor to scale the input before writing to the histogram.
	*/
	template<ContainerTrait::ValueRange<value_type> R>
	requires(!SparseBin::SparseBinRange<R>)
	constexpr void operator()(R&& input, const auto x, const auto y) {
		using Range_t = std::ranges::range_value_t<R>;
		auto input_range = input
			| std::views::enumerate
			| std::views::filter([](const auto& bin) constexpr noexcept {
				return std::get<1u>(bin) != Range_t { 0 };
			}) | std::views::transform([](const auto& bin) constexpr noexcept {
				const auto [region, value] = bin;
				return bin_type {
					.Region = static_cast<Format::Region_t>(region),
					.Value = value
				};
			});
		this->insertBin(input_range, x, y);
	}
	template<std::ranges::input_range R, typename T>
	requires(!SparseBin::SparseBinRange<R> && std::floating_point<value_type>)
	constexpr void operator()(R&& input, const auto x, const auto y, const T factor) {
		//resolves to the function above ^^^
		(*this)(input | std::views::transform(
			[factor](const auto value) constexpr noexcept { return SCALE_TRANSFORM; }), x, y);
	}

	/**
	 * @brief Read bins from this sparse histogram given histogram index.
	 * 
	 * @param x, y The index to the histogram whose bins are to be read.
	 * 
	 * @return The view into the histogram bins for the current coordinate.
	*/
	constexpr ConstBinView operator()(const auto x, const auto y) const noexcept {
		const auto [first, last] = this->getBinBound(x, y);
		const auto bin_it = this->Bin[y].cbegin();
		return ConstBinView(bin_it + first, bin_it + last);
	}

};

template<typename T>
class BasicSparse<T, true> : public BasicSparseInternalStorage<T> {
private:

	//Pretty straight-forward, sort every histogram by region.
	void sortStorage();

public:

	constexpr BasicSparse() noexcept = default;

	//Convert a unsorted sparse histogram to a sorted one.
	//CONSIDER: Can add other constructors if needed.
	BasicSparse& operator=(BasicSparse<T, false>&&);

	constexpr ~BasicSparse() = default;

	/**
	 * @brief Compare equality of two sparse histograms.
	 *
	 * @param comp The other sparse histogram.
	 *
	 * @return True if two histograms are identical if all offsets and bins are equal.
	*/
	bool operator==(const BasicSparse&) const;

	FRIEND_DENSE_EQ_SPARSE;

};

template<typename T>
class BasicSparse<T, false> : public BasicSparseInternalStorage<T> {
private:

	friend class BasicSparse<T, true>;

	using Base = BasicSparseInternalStorage<T>;

	using typename Base::bin_type,
		typename Base::value_type;

public:

	using Base::operator();

	constexpr BasicSparse() noexcept = default;

	constexpr ~BasicSparse() = default;

	/**
	 * @brief Push some bins from a sparse histogram into the current one.
	 * 
	 * @tparam R The type of input range.
	 * @tparam T The type of factor.
	 * 
	 * @param input The input range containing bins to be pushed.
	 * @param x,y The index to the histogram.
	 * @param factor Provide a factor to scale the input before writing to the histogram.
	*/
	template<SparseBin::SparseBinRangeValue<value_type> R>
	constexpr void operator()(R&& input, const auto x, const auto y) {
		this->insertBin(std::forward<R>(input), x, y);
	}
	template<SparseBin::SparseBinRange R, typename T>
	requires(std::floating_point<value_type>)
	constexpr void operator()(R&& input, const auto x, const auto y, const T factor) {
		//resolves to the function above ^^^
		(*this)(input | std::views::transform([factor](const auto& bin) constexpr noexcept {
			const auto [region, value] = bin;
			return bin_type {
				.Region = region,
				.Value = SCALE_TRANSFORM
			};
		}), x, y);
	}

};
template<typename T>
using BasicSparseSorted = BasicSparse<T, true>;
template<typename T>
using BasicSparseUnsorted = BasicSparse<T, false>;

using SparseSorted = BasicSparseSorted<Format::Bin_t>;
using SparseNormSorted = BasicSparseSorted<Format::NormBin_t>;
using SparseUnsorted = BasicSparseUnsorted<Format::Bin_t>;
using SparseNormUnsorted = BasicSparseUnsorted<Format::NormBin_t>;

#undef SCALE_TRANSFORM

#undef DENSE_EQ_SPARSE
#undef FRIEND_DENSE_EQ_SPARSE

}

}