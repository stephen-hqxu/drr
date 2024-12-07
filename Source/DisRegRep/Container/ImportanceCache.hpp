#pragma once

#include "SparseMatrixElement.hpp"

#include <DisRegRep/Arithmetic.hpp>
#include <DisRegRep/Type.hpp>

#include <vector>

#include <algorithm>
#include <execution>
#include <functional>
#include <ranges>

#include <limits>
#include <utility>

#include <concepts>
#include <type_traits>

#include <cassert>

/**
 * @brief Cache intermediate values of region importance before writing to the output.
 */
namespace DisRegRep::Container::ImportanceCache {

namespace Internal_ {

using DenseEntryType = Type::RegionImportance; /**< Type of entry in dense importance cache. */

/**
 * `Op` is a binary operator on dense entry values.
 */
template<typename Op>
concept DenseEntryBinaryOperator = std::is_convertible_v<std::invoke_result_t<Op, DenseEntryType, DenseEntryType>, DenseEntryType>;

/**
 * `Modifier` modifies `Cache` in-place given a sparse importance matrix element `Importance`.
 */
template<typename Modifier, typename Cache, typename Importance>
concept CacheModifier = SparseMatrixElement::ImportanceInputRange<Importance>
					 && std::invocable<Modifier, std::add_lvalue_reference_t<Cache>, std::ranges::range_const_reference_t<Importance>>;

//In-place modifies cache with a modifier member function given a range of sparse importance matrix element.
template<
	typename Cache,
	SparseMatrixElement::ImportanceInputRange Importance,
	CacheModifier<Cache, Importance> Modifier
>
void modify(Cache& cache, Importance&& importance, Modifier&& modifier) {
	using std::ranges::cbegin, std::ranges::cend,
		std::for_each, std::execution::unseq,
		std::invoke;
	for_each(unseq, cbegin(importance), cend(importance),
		[&cache, &modifier](const auto& element) noexcept { invoke(modifier, cache, element); });
}

}

/**
 * @brief A dense cache is a contiguous linear array that stores importance of each region at the corresponding index based on region
 * identifier.
 */
class Dense {
public:

	using EntryType = Internal_::DenseEntryType;
	using SizeType = Type::RegionIdentifier;

private:

	using ContainerType = std::vector<EntryType>;

	ContainerType Importance;

	//Modify one region by some amount.
	template<Internal_::DenseEntryBinaryOperator Op>
	constexpr void modify(const SizeType region_id, Op&& op, const SizeType amount) noexcept {
		EntryType& importance = this->Importance[region_id];
		if constexpr (std::is_same_v<Op, std::minus<>>) {
			assert(importance >= amount);
		}
		importance = std::invoke(std::forward<Op>(op), importance, amount);
	}

	//Modify one region by a given sparse importance matrix element.
	template<Internal_::DenseEntryBinaryOperator Op>
	constexpr void modify(const SparseMatrixElement::Importance& importance, Op&& op) noexcept {
		const auto [region_id, value] = importance;
		this->modify(region_id, std::forward<Op>(op), value);
	}

	//Modify all regions by some amount.
	template<Internal_::DenseEntryBinaryOperator Op, Type::RegionImportanceInputRange Amount>
	void modify(Op&& op, Amount&& amount) {
		if constexpr (std::is_same_v<Op, std::minus<>>) {
			using std::ranges::all_of, std::views::zip;
			assert(all_of(zip(this->Importance, amount), std::ranges::greater_equal {}));
		}
		Arithmetic::addRange(this->Importance, std::forward<Op>(op), std::forward<Amount>(amount), this->Importance.begin());
	}

	//Modify some regions by some amount.
	template<
		SparseMatrixElement::ImportanceInputRange Importance,
		Internal_::CacheModifier<Dense&, Importance> Modifier
	>
	void modify(Importance&& importance, Modifier&& modifier) {
		Internal_::modify(*this, std::forward<Importance>(importance), std::forward<Modifier>(modifier));
	}

public:

	constexpr Dense() noexcept = default;

	Dense(const Dense&) = delete;

	Dense(Dense&&) = delete;

	Dense& operator=(const Dense&) = delete;

	Dense& operator=(Dense&&) = delete;

	constexpr ~Dense() = default;

	/**
	 * @brief Resize dense cache.
	 *
	 * @param region_count The maximum number of region identifiers to be held by this cache.
	 */
	constexpr void resize(const SizeType region_count) {
		this->Importance.resize(region_count);
	}

	/**
	 * @brief Clear all contents in the cache and reset importance of all regions to zero.
	 */
	constexpr void clear() noexcept {
		std::ranges::fill(this->Importance, EntryType {});
	}

	/**
	 * @brief Increment the importance of a region by one.
	 *
	 * @param region_id Identifier of region whose importance is to be incremented.
	 */
	constexpr void increment(const SizeType region_id) noexcept {
		this->modify(region_id, std::plus {}, 1U);
	}

	/**
	 * @brief Increment the importance of a region by some amount.
	 *
	 * @param importance A sparse importance matrix element used for incrementation.
	 */
	constexpr void increment(const SparseMatrixElement::Importance& importance) noexcept {
		this->modify(importance, std::plus {});
	}

	/**
	 * @brief Increment the importance of all regions by some amount.
	 *
	 * @tparam Amount A range of importance for region at each index.
	 *
	 * @param amount The size of this range must be no less than the size of the cache.
	 */
	template<Type::RegionImportanceInputRange Amount>
	void increment(Amount&& amount) {
		this->modify(std::plus {}, std::forward<Amount>(amount));
	}

	/**
	 * @brief Increment the importance of some regions by some amount.
	 *
	 * @tparam Importance A range of sparse importance matrix element.
	 *
	 * @param importance Each specifies the region identifier and the amount of importance to increment.
	 */
	template<SparseMatrixElement::ImportanceInputRange Importance>
	void increment(Importance&& importance) {
		this->modify(std::forward<Importance>(importance),
			std::mem_fn(static_cast<void (Dense::*)(const SparseMatrixElement::Importance&)>(&Dense::increment)));
	}

	/**
	 * @brief Decrement the importance of a region by one.
	 *
	 * @param region_id Identifier of region whose importance is to be decremented.
	 */
	constexpr void decrement(const SizeType region_id) noexcept {
		this->modify(region_id, std::minus {}, 1U);
	}

	/**
	 * @brief Decrement the importance of a region by some amount.
	 *
	 * @param importance A sparse importance matrix element used for decrementation.
	 */
	constexpr void decrement(const SparseMatrixElement::Importance& importance) noexcept {
		this->modify(importance, std::minus {});
	}

	/**
	 * @brief Decrement the importance of all regions by some amount.
	 *
	 * @tparam Amount A range of importance for region at each index.
	 *
	 * @param amount  The size of this range must be no less than the size of the cache.
	 */
	template<Type::RegionImportanceInputRange Amount>
	void decrement(Amount&& amount) {
		this->modify(std::minus {}, std::forward<Amount>(amount));
	}

	/**
	 * @brief Decrement the importance of some regions by some amount.
	 *
	 * @tparam Importance A range of sparse importance matrix element.
	 *
	 * @param importance Each specifies the region identifier and the amount of importance to decrement.
	 */
	template<SparseMatrixElement::ImportanceInputRange Importance>
	void decrement(Importance&& importance) {
		this->modify(std::forward<Importance>(importance),
			std::mem_fn(static_cast<void (Dense::*)(const SparseMatrixElement::Importance&)>(&Dense::decrement)));
	}

};

/**
 * @brief A sparse cache collects two contiguous arrays, one stores sparse importance entries, the other stores offsets into the sparse
 * array given region identifier.
 */
class Sparse {
public:

	using EntryType = SparseMatrixElement::Importance;
	using OffsetType = Dense::SizeType;
	using SizeType = OffsetType;

private:

	using EntryContainerType = std::vector<EntryType>;
	using OffsetContainerType = std::vector<OffsetType>;

	//A special offset to indicate a region identifier does not exist in the cache.
	static constexpr auto NoEntryOffset = std::numeric_limits<OffsetType>::max();

	EntryContainerType Importance;
	OffsetContainerType Offset;

	//Modify some regions by some amount.
	template<
		SparseMatrixElement::ImportanceInputRange Importance,
		Internal_::CacheModifier<Sparse&, Importance> Modifier
	>
	void modify(Importance&& importance, Modifier&& modifier) {
		Internal_::modify(*this, std::forward<Importance>(importance), std::forward<Modifier>(modifier));
	}

public:

	constexpr Sparse() noexcept = default;

	Sparse(const Sparse&) = delete;

	Sparse(Sparse&&) = delete;

	Sparse& operator=(const Sparse&) = delete;

	Sparse& operator=(Sparse&&) = delete;

	constexpr ~Sparse() = default;

	/**
	 * @brief Resize sparse cache.
	 *
	 * @param region_count The maximum number of region identifiers to be held by this cache.
	 */
	constexpr void resize(const SizeType region_count) {
		this->Offset.resize(region_count, Sparse::NoEntryOffset);
	}

	/**
	 * @brief Clear all contents in the cache.
	 */
	constexpr void clear() noexcept {
		this->Importance.clear();
		std::ranges::fill(this->Offset, Sparse::NoEntryOffset);
	}

	/**
	 * @brief Increment importance of a region by a given amount specified in a sparse importance matrix element.
	 *
	 * @param importance An element used for incrementation.
	 */
	void increment(const EntryType&);

	/**
	 * @brief Increment importance of a region by one.
	 *
	 * @param region_id Identifier of region whose importance is to be incremented.
	 */
	void increment(SizeType);

	/**
	 * @brief Increment importance of some regions by some amount.
	 * 
	 * @tparam Importance A range of sparse importance matrix element.
	 * @param importance Each specifies the region identifier and the amount of importance to increment.
	 */
	template<SparseMatrixElement::ImportanceInputRange Importance>
	void increment(Importance&& importance) {
		this->modify(
			std::forward<Importance>(importance), std::mem_fn(static_cast<void (Sparse::*)(const EntryType&)>(&Sparse::increment)));
	}

	/**
	 * @brief Decrement importance of a region by a given amount specified in a sparse importance matrix element.
	 *
	 * @param importance An element used for decrementation.
	 */
	void decrement(const EntryType&);

	/**
	 * @brief Decrement importance of a region by one.
	 *
	 * @param region_id Identifier of region whose importance is to be decremented.
	 */
	void decrement(SizeType);

	/**
	 * @brief Decrement importance of some regions by some amount.
	 * 
	 * @tparam Importance A range of sparse importance matrix element.
	 * @param importance Each specifies the region identifier and the amount of importance to decrement.
	 */
	template<SparseMatrixElement::ImportanceInputRange Importance>
	void decrement(Importance&& importance) {
		this->modify(
			std::forward<Importance>(importance), std::mem_fn(static_cast<void (Sparse::*)(const EntryType&)>(&Sparse::decrement)));
	}

};

}