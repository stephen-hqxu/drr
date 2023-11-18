#pragma once

#include "Maths/Arithmetic.hpp"
#include "Maths/Indexer.hpp"

#include <array>
#include <vector>
#include <span>
#include <ranges>

#include <utility>
#include <concepts>

#include <cassert>
#include <cstdint>

namespace DisRegRep {

/**
 * @brief Standard type for filter format.
*/
namespace Format {

using SizeVec2 = std::array<size_t, 2u>;/**< An array of two size_t's. */
using Radius_t = std::uint16_t;/**< Type of radius. */

using Region_t = std::uint8_t;/**< Type of region, which is also the pixel format of region map. */
using Bin_t = std::uint16_t;/**< Format of bin that stores region count. */
using NormBin_t = float;/**< Format of bin that stores normalised weight. */

using DenseSingleHistogram = std::vector<Bin_t>;/**< Dense matrix of unnormalised bins. */
using DenseNormSingleHistogram = std::vector<NormBin_t>;/**< Sparse matrix of normalised bins. */

using DefaultDenseHistogramIndexer = Indexer<1, 2, 0>;/**< Default indexing order of histogram. */

/**
 * @brief Single histogram but with sparse matrix.
 * @tparam TBinValue The type of value of bin.
*/
template<typename TBinValue>
class BasicSparseSingleHistogram {
public:

	/**
	 * @brief Type of bin.
	*/
	struct BinType {

		Format::Region_t Region;/**< The region this bin is storing. */
		TBinValue Value;/**< Bin value for this region. */

	};
	using BinView = std::span<const BinType>;/**< View into bins in a histogram. */

private:

	using BinOffset_t = std::uint32_t;

	std::vector<BinOffset_t> Offset;
	std::vector<BinType> Bin;

public:

	constexpr BasicSparseSingleHistogram() noexcept {
		this->Offset.push_back(0u);
	}

	constexpr ~BasicSparseSingleHistogram() = default;

	/**
	 * @brief Get the number of histogram stored.
	 * 
	 * @return The size of single histogram.
	*/
	constexpr size_t size() const noexcept {
		return this->Offset.size();
	}

	/**
	 * @brief Resize the histogram.
	 * 
	 * @param new_size The number number of histogram resized to.
	*/
	constexpr void resize(const size_t new_size) {
		//we keep one extra offset at the end to indicate the total number of bins.
		this->Offset.resize(new_size + 1u);
	}

	/**
	 * @brief Push some bins into a new histogram.
	 * 
	 * @tparam R The type of input range.
	 * @tparam RT The range type, will be deduced.
	 * @param histogram_idx The index to the histogram whose bins are belonging to.
	 * The behaviour is undefined if bins are not pushed contiguously.
	 * @param input The input range containing bins to be pushed.
	*/
	template<std::ranges::input_range R, typename RT = std::ranges::range_value_t<R>>
	requires(std::same_as<RT, BinType>)
	constexpr void pushBin(const size_t histogram_idx, R&& input) {
		this->Bin.insert_range(this->Bin.cend(), std::forward<R>(input));
		this->Offset[histogram_idx + 1u] = static_cast<BinOffset_t>(this->Bin.size());
	}

	/**
	 * @brief Read bins for histogram given histogram index.
	 * 
	 * @param histogram_idx The index to the histogram whose bins are to be read.
	 * 
	 * @return A view into the bins of requested histogram.
	*/
	constexpr BinView readBin(const size_t histogram_idx) const {
		const size_t bin_start_idx = this->Offset[histogram_idx],
			bin_end_idx = this->Offset[histogram_idx + 1u];
		const auto bin_it = this->Bin.cbegin();
		return BinView(bin_it + bin_start_idx, bin_it + bin_end_idx);
	}

};
using SparseSingleHistogram = BasicSparseSingleHistogram<Bin_t>;/**< Sparse matrix of unnormalised bins. */
using SparseNormSingleHistogram = BasicSparseSingleHistogram<NormBin_t>;/***< Sparse matrix of normalised bins. /

/**
 * @brief A region map.
*/
class RegionMap {
private:

	std::vector<Region_t> Map;/**< Region map data. */
	SizeVec2 Dimension { };/**< The dimension of region map. */

	using RegionMapIndexer_t = Indexer<0, 1>;
	RegionMapIndexer_t RegionMapIndexer;/**< Default indexing order of region map. */

public:

	size_t RegionCount { };/**< The total number of contiguous region presented on region map. */

	constexpr RegionMap() = default;

	constexpr ~RegionMap() = default;

	/**
	 * @brief Get the region value by pixel coordinate.
	 *
	 * @tparam T... The type of index, must be integer.
	 * @param axis... The axis indices into the region map.
	 *
	 * @return The region value.
	*/
	template<typename... T>
	constexpr Region_t& operator()(const T... axis) noexcept {
		return this->Map[this->RegionMapIndexer(axis...)];
	}
	template<typename... T>
	constexpr Region_t operator()(const T... axis) const noexcept {
		return this->Map[this->RegionMapIndexer(axis...)];
	}

	/**
	 * @brief Resize region map to new dimension.
	 *
	 * @param dimension The new dimension of region map.
	 * 
	 * @return New size.
	*/
	constexpr size_t resizeToDimension(const SizeVec2& dimension) {
		const auto [x, y] = dimension;
		const size_t new_size = Arithmetic::horizontalProduct(dimension);

		this->Map.resize(new_size);
		this->Dimension = dimension;
		this->RegionMapIndexer = RegionMapIndexer_t(x, y);
		return new_size;
	}

	/**
	 * @brief Get the size of region map.
	 *
	 * @return The total number of pixel this region map has.
	*/
	constexpr size_t size() const noexcept {
		const size_t s = this->Map.size();
		assert(s == Arithmetic::horizontalProduct(this->Dimension));
		return s;
	}

	/**
	 * @brief Get the dimension of region map.
	 * 
	 * @return The dimension.
	*/
	constexpr const SizeVec2& dimension() const noexcept {
		return this->Dimension;
	}

	////////////////////////////
	/// Container functions
	///////////////////////////

	auto begin() noexcept {
		return this->Map.begin();
	}
	auto end() noexcept {
		return this->Map.end();
	}

};

}

}