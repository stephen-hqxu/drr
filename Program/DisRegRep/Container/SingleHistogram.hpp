#pragma once

#include <DisRegRep/ProgramExport.hpp>
#include "../Format.hpp"
#include "../Maths/Indexer.hpp"

#include <vector>
#include <span>

#include <ranges>
#include <functional>

#include <concepts>
#include <cstdint>

namespace DisRegRep {

/**
 * @brief A single histogram is a matrix where each element points to a histogram, which is an array of bins.
*/
namespace SingleHistogram {

/**
 * @brief A dense single histogram uses a 3D matrix.
 * 
 * @tparam TBin The bin type.
*/
template<typename TBin>
class DRR_API BasicDense {
public:

	using value_type = TBin;

private:

	std::vector<value_type> Histogram;

	using DenseIndexer_t = Indexer<1, 2, 0>;/**< Default indexing order of histogram. */
	DenseIndexer_t DenseIndexer;

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
	bool operator==(const BasicDense& comp) const;

	/**
	 * @brief Reshape the dimension of histogram.
	 * If dimension is shrunken, extra data will be trimmed.
	 * 
	 * @param dimension The new dimension.
	 * @param bin_count The number of bin per histogram.
	*/
	void reshape(const Format::SizeVec2& dimension, const size_t bin_count);

	/**
	 * @brief Get the view into a histogram.
	 * 
	 * @param x The x index.
	 * @param y The y index.
	 * @param bin The bin index.
	 * 
	 * @return The view of all bins in a histogram.
	*/
	constexpr BinView operator()(const auto x, const auto y, const auto bin = 0u) noexcept {
		const ConstBinView view_const = const_cast<const BasicDense&>(*this)(x, y, bin);
		return BinView(const_cast<BinView::pointer>(view_const.data()), view_const.size());
	}
	constexpr ConstBinView operator()(const auto x, const auto y, const auto bin = 0u) const noexcept {
		return ConstBinView(this->Histogram.cbegin() + this->DenseIndexer(x, y, bin), this->DenseIndexer.extent(2u));
	}

};
using Dense = BasicDense<Format::Bin_t>;
using DenseNorm = BasicDense<Format::NormBin_t>;

template<typename T>
concept DenseInstance = requires {
	{ BasicDense<typename T::value_type> { } } -> std::same_as<T>;
};

/**
 * @brief A sparse single histogram is a 2D matrix of arrays with, useful if the bin axis is sparse.
 * 
 * @tparam TBin The bin type.
*/
template<typename TBin>
class DRR_API BasicSparse {
public:

	using value_type = TBin;

	/**
	 * @brief Type of bin.
	*/
	struct BinType {

		Format::Region_t Region;/**< The region this bin is storing. */
		value_type Value;/**< Bin value for this region. */

		constexpr bool operator==(const BinType&) const noexcept = default;

	};
	using BinView = std::span<const BinType>;

private:

	using BinOffset_t = std::uint32_t;

	std::vector<BinOffset_t> Offset;
	//List-of-List sparse matrix format, this is better than List-of-Flattened-List
	//	for allowing constructing the matrix in row-major or column-major order at the same time.
	std::vector<std::vector<BinType>> Bin;

	using BinOffsetIndexer_t = Indexer<0, 1>;
	BinOffsetIndexer_t OffsetIndexer;

public:

	constexpr BasicSparse() noexcept = default;

	//Initialise with dimension.
	constexpr BasicSparse(const Format::SizeVec2& dimension) {
		this->reshape(dimension);
	}

	BasicSparse(const BasicSparse&) = delete;

	constexpr BasicSparse(BasicSparse&&) noexcept = default;

	constexpr ~BasicSparse() = default;

	/**
	 * @brief Compare equality of two sparse histograms.
	 *
	 * @param comp The other sparse histogram.
	 *
	 * @return True if two histograms are identical if all offsets and bins are equal.
	*/
	bool operator==(const BasicSparse& comp) const;

	/**
	 * @brief Get the number of histogram stored.
	 *
	 * @return The size of single histogram.
	*/
	constexpr size_t size() const noexcept {
		return this->Offset.size();
	}

	/**
	 * @brief Reshape the histogram.
	 * All contents in the histogram will be cleared.
	 *
	 * @param dimension The new dimension for the histogram.
	*/
	void reshape(Format::SizeVec2 dimension);

	/**
	 * @brief Push some bins from a dense histogram into the current sparse histogram.
	 * 
	 * @tparam R The type of input range.
	 * @param input The input range containing bins to be pushed.
	 * @param x, y The index to the histogram whose bins are belonging to.
	 * The behaviour is undefined if bins are not pushed contiguously in y axis.
	 * @param proj Project range value before writing to the histogram.
	*/
	template<std::ranges::input_range R, typename Proj = std::identity>
	constexpr void pushDense(R&& input, const auto x, const auto y, Proj&& proj = { }) {
		using Range_t = std::ranges::range_value_t<R>;
		const auto input_range = input
			| std::views::enumerate
			| std::views::filter([](const auto& bin) constexpr noexcept {
				return bin.second != Range_t { 0 };
			}) | std::views::transform([&proj](const auto& bin) constexpr noexcept {
				const auto [region, value] = bin;
				return BinType {
					.Region = static_cast<Format::Region_t>(region),
					.Value = std::forward<Proj>(proj)(value)
				};
			});
		
		auto& bin_y = this->Bin[y];
		bin_y.insert_range(bin_y.cend(), input_range);
		this->Offset[this->OffsetIndexer(x + 1, y)] = static_cast<BinOffset_t>(bin_y.size());
	}

	/**
	 * @brief Read bins for a dense histogram given histogram index.
	 * 
	 * @param x, y The index to the histogram whose bins are to be read.
	 * 
	 * @return The view into the histogram bins for the current coordinate.
	*/
	constexpr BinView readSparse(const auto x, const auto y) const {
		//as offset matrix is row-major linear, we are doing this to avoid recalculating the offset linear index twice
		const size_t offset_idx = this->OffsetIndexer(x, y),
			bin_start_idx = this->Offset[offset_idx],
			//this is equivalent to `this->OffsetIndexer(x + 1, y)`
			bin_end_idx = this->Offset[offset_idx + 1u];
		const auto bin_it = this->Bin[y].cbegin();
		return BinView(bin_it + bin_start_idx, bin_it + bin_end_idx);
	}

};
using Sparse = BasicSparse<Format::Bin_t>;
using SparseNorm = BasicSparse<Format::NormBin_t>;

template<typename T>
concept SparseInstance = requires {
	{ BasicSparse<typename T::value_type> { } } -> std::same_as<T>;
};

}

}