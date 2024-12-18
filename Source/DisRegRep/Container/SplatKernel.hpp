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
 * @brief Stores region importance of the convolution kernel for splatting region features.
 *
 * For all kinds of kernels, two fundamental functions are provided: increment and decrement. They work by given region identifier(s)
 * and the amount of importance to be increased/decreased. To ensure high efficiency, no exception handling is provided for the inputs.
 * Specifically for decrement, the behaviour is undefined if importance for any region underflows.
 */
namespace DisRegRep::Container::SplatKernel {

namespace Internal_ {

using DenseValueType = Type::RegionImportance; /**< Value type of kernel using dense matrix. */

/**
 * `Op` is a binary operator on dense kernel values.
 */
template<typename Op>
concept DenseKernelBinaryOperator = std::is_convertible_v<std::invoke_result_t<Op, DenseValueType, DenseValueType>, DenseValueType>;

/**
 * `Modifier` modifies `Kernel` in-place given a sparse importance matrix element `Importance`.
 */
template<typename Modifier, typename Kernel, typename Importance>
concept KernelModifier = SparseMatrixElement::ImportanceInputRange<Importance>
					 && std::invocable<Modifier, std::add_lvalue_reference_t<Kernel>, std::ranges::range_const_reference_t<Importance>>;

//In-place modifies kernel with a modifier member function given a range of sparse importance matrix element.
template<
	typename Kernel,
	SparseMatrixElement::ImportanceInputRange Importance,
	KernelModifier<Kernel, Importance> Modifier
>
void modify(Kernel& kernel, Importance&& importance, Modifier&& modifier) {
	using std::ranges::cbegin, std::ranges::cend,
		std::for_each, std::execution::unseq,
		std::invoke;
	for_each(unseq, cbegin(importance), cend(importance),
		[&kernel, &modifier](const auto& element) noexcept { invoke(modifier, kernel, element); });
}

}

/**
 * @brief A dense kernel is a contiguous linear array that stores importance of each region at the corresponding index based on region
 * identifier.
 */
class Dense {
public:

	using ValueType = Internal_::DenseValueType;
	using SizeType = Type::RegionIdentifier;

private:

	using ContainerType = std::vector<ValueType>;

	ContainerType Importance;

	//Modify one region by some amount.
	template<Internal_::DenseKernelBinaryOperator Op>
	constexpr void modify(const SizeType region_id, Op&& op, const ValueType amount) noexcept {
		ValueType& importance = this->Importance[region_id];
		if constexpr (std::is_same_v<Op, std::minus<>>) {
			assert(importance >= amount);
		}
		importance = std::invoke(std::forward<Op>(op), importance, amount);
	}

	//Modify one region by a given sparse importance matrix element.
	template<Internal_::DenseKernelBinaryOperator Op>
	constexpr void modify(const SparseMatrixElement::Importance& importance, Op&& op) noexcept {
		const auto [region_id, value] = importance;
		this->modify(region_id, std::forward<Op>(op), value);
	}

	//Modify all regions by some amount.
	template<Internal_::DenseKernelBinaryOperator Op, Type::RegionImportanceInputRange Amount>
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
		Internal_::KernelModifier<Dense&, Importance> Modifier
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
	 * @brief Resize dense kernel.
	 *
	 * @param region_count The maximum number of region identifiers to be held by this kernel.
	 */
	constexpr void resize(const SizeType region_count) {
		this->Importance.resize(region_count);
	}

	/**
	 * @brief Clear all contents in the kernel and reset importance of all regions to zero.
	 */
	constexpr void clear() noexcept {
		std::ranges::fill(this->Importance, ValueType {});
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
	 * @param amount The size of this range must be no less than the size of the kernel.
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
	 * @param amount The size of this range must be no less than the size of the kernel.
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
 * @brief A sparse kernel collects two contiguous arrays, one stores sparse importance entries, the other stores offsets into the sparse
 * array given region identifier.
 */
class Sparse {
public:

	using ValueType = SparseMatrixElement::Importance;
	using OffsetType = Dense::SizeType;
	using SizeType = OffsetType;

private:

	using ValueContainerType = std::vector<ValueType>;
	using OffsetContainerType = std::vector<OffsetType>;

	//A special offset to indicate a region identifier does not exist in the kernel.
	static constexpr auto NoValueOffset = std::numeric_limits<OffsetType>::max();

	ValueContainerType Importance;
	OffsetContainerType Offset;

	//Modify some regions by some amount.
	template<
		SparseMatrixElement::ImportanceInputRange Importance,
		Internal_::KernelModifier<Sparse&, Importance> Modifier
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
	 * @brief Resize sparse kernel.
	 *
	 * @param region_count The maximum number of region identifiers to be held by this kernel.
	 */
	constexpr void resize(const SizeType region_count) {
		this->Offset.resize(region_count, Sparse::NoValueOffset);
	}

	/**
	 * @brief Clear all contents in the kernel.
	 */
	constexpr void clear() noexcept {
		this->Importance.clear();
		std::ranges::fill(this->Offset, Sparse::NoValueOffset);
	}

	/**
	 * @brief Increment importance of a region by a given amount specified in a sparse importance matrix element.
	 *
	 * @param importance An element used for incrementation.
	 */
	void increment(const ValueType&);

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
			std::forward<Importance>(importance), std::mem_fn(static_cast<void (Sparse::*)(const ValueType&)>(&Sparse::increment)));
	}

	/**
	 * @brief Decrement importance of a region by a given amount specified in a sparse importance matrix element.
	 *
	 * @param importance An element used for decrementation.
	 */
	void decrement(const ValueType&);

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
			std::forward<Importance>(importance), std::mem_fn(static_cast<void (Sparse::*)(const ValueType&)>(&Sparse::decrement)));
	}

};

}