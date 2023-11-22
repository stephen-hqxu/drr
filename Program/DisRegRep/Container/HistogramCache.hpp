#pragma once

#include <DisRegRep/ProgramExport.hpp>
#include "ContainerTrait.hpp"
#include "SparseBin.hpp"

#include "../Format.hpp"
#include "../Maths/Arithmetic.hpp"

#include <vector>
#include <algorithm>
#include <functional>

#include <limits>

namespace DisRegRep {

/**
 * @brief A cache for generation of single histogram.
*/
namespace HistogramCache {

/**
 * @brief Dense histogram cache.
*/
class Dense {
public:

	using bin_type = Format::Bin_t;

private:

	using Histogram_t = std::vector<bin_type>;
	Histogram_t Histogram;

public:

	using const_iterator = Histogram_t::const_iterator;

	constexpr Dense() noexcept = default;

	Dense(const Dense&) = delete;

	constexpr Dense(Dense&&) noexcept = default;

	constexpr ~Dense() = default;

	/**
	 * @brief Resize dense cache.
	 * 
	 * @param region_count The maximum number of region to be held by this cache.
	*/
	constexpr void resize(const Format::Region_t region_count) {
		this->Histogram.resize(region_count);
	}

	/**
	 * @brief Clear all contents in the cache.
	*/
	constexpr void clear() noexcept {
		std::ranges::fill(this->Histogram, bin_type { });
	}

	/**
	 * @brief Increment the count of a bin.
	 * 
	 * @tparam R The input range type.
	 * 
	 * @param region The region to be incremented by one.
	 * @param input A range of dense bins that have the same number of region count as this cache,
	 * each corresponding bin will be incremented.
	*/
	constexpr void increment(const Format::Region_t region) noexcept {
		this->Histogram[region]++;
	}
	template<ContainerTrait::ValueRange<bin_type> R>
	void increment(R&& input) {
		Arithmetic::addRange(this->Histogram, std::plus { }, input, this->Histogram.begin());
	}

	/**
	 * @brief Increment the count with sparse bins.
	 * 
	 * @tparam R The input range type.
	 * 
	 * @param bin The sparse bin to be incremented.
	 * @param input A range of sparse bins to be incremented.
	*/
	constexpr void increment(const SparseBin::Bin& bin) noexcept {
		const auto [region, value] = bin;
		this->Histogram[region] += value;
	}
	template<SparseBin::SparseBinRangeValue<bin_type> R>
	void increment(R&& input) {
		std::ranges::for_each(input, [this](const auto& bin) noexcept { this->increment(bin); });
	}

	/**
	 * @brief Decrement the count of a bin.
	 * 
	 * @tparam R The input range type.
	 * 
	 * @param region The region to be decremented by one.
	 * @param input A range of dense bins to be decremented.
	*/
	constexpr void decrement(const Format::Region_t region) noexcept {
		this->Histogram[region]--;
	}
	template<ContainerTrait::ValueRange<bin_type> R>
	void decrement(R&& input) {
		Arithmetic::addRange(this->Histogram, std::minus { }, input, this->Histogram.begin());
	}

	/**
	 * @brief Decrement the count with sparse bins.
	 *
	 * @tparam R The input range type.
	 *
	 * @param bin The sparse bin to be decremented.
	 * @param input A range of sparse bins to be decremented.
	*/
	constexpr void decrement(const SparseBin::Bin& bin) noexcept {
		const auto [region, value] = bin;
		this->Histogram[region] -= value;
	}
	template<SparseBin::SparseBinRangeValue<bin_type> R>
	void decrement(R&& input) {
		std::ranges::for_each(input, [this](const auto& bin) noexcept { this->decrement(bin); });
	}

	//////////////////////////
	/// Container function
	/////////////////////////

	constexpr const_iterator begin() const noexcept {
		return this->Histogram.cbegin();
	}

	constexpr const_iterator end() const noexcept {
		return this->Histogram.cend();
	}

};

/**
 * @brief Sparse histogram cache, utilises sparse set data structure.
*/
class Sparse {
public:

	using bin_type = SparseBin::Bin;
	using index_type = Format::Region_t;

private:

	using Dense_t = std::vector<bin_type>;
	using Sparse_t = std::vector<index_type>;

	//This will be used to indicate if a region exists in the dense array.
	constexpr static auto NoEntryIndex = std::numeric_limits<index_type>::max();

	//The dense array stores different regions.
	Dense_t DenseSet;
	//The sparse arrays stores offsets into the dense array given region ID.
	Sparse_t SparseSet;

public:

	using const_iterator = Dense_t::const_iterator;

	constexpr Sparse() noexcept = default;

	Sparse(const Sparse&) = delete;

	constexpr Sparse(Sparse&&) noexcept = default;

	constexpr ~Sparse() = default;

	/**
	 * @brief Resize the sparse cache.
	 *
	 * @param region_count The region count to be used.
	*/
	constexpr void resize(const Format::Region_t region_count) {
		this->SparseSet.resize(region_count, Sparse::NoEntryIndex);
	}

	/**
	 * @brief Clear all content in the cache.
	*/
	constexpr void clear() noexcept {
		this->DenseSet.clear();
		std::ranges::fill(this->SparseSet, Sparse::NoEntryIndex);
	}

	/**
	 * @brief Add a bin entry.
	 *
	 * @tparam R The range type.
	 *
	 * @param bin The bin that will be added.
	 * @param region The region to be added. It will be incremented by one.
	 * @param input A range of bins to be incremented.
	*/
	DRR_API void increment(const bin_type&);
	DRR_API void increment(Format::Region_t);

	template<SparseBin::SparseBinRangeValue<bin_type::value_type> R>
	void increment(R&& input) {
		std::ranges::for_each(input, [this](const auto& bin) { this->increment(bin); });
	}

	/**
	 * @brief Remove a bin entry.
	 * If the bin will become empty after decrementing, bin will be erased from the cache and dictionary will
	 * be rehashed. Those operations are expensive, so don't call this function too often. In our implementation
	 * erasure rarely happens, benchmark shows this is still the best method.
	 * 
	 * The behaviour is undefined if bin does not exist.
	 *
	 * @tparam R The range type.
	 *
	 * @param bin The bin that will be removed.
	 * @param region The region to be removed. It will be decremented by one.
	 * @param input The range of bins to be decremented.
	*/
	DRR_API void decrement(const bin_type&);
	DRR_API void decrement(Format::Region_t);

	template<SparseBin::SparseBinRangeValue<bin_type::value_type> R>
	void decrement(R&& input) {
		std::ranges::for_each(input, [this](const auto& bin) { this->decrement(bin); });
	}

	/////////////////////////
	/// Container function
	////////////////////////

	constexpr const_iterator begin() const noexcept {
		return this->DenseSet.cbegin();
	}

	constexpr const_iterator end() const noexcept {
		return this->DenseSet.cend();
	}

};

}

}