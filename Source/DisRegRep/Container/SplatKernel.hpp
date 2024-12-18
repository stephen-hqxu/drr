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

/**
 * `Importance` is a range of importance.
 */
template<typename Importance>
concept DenseImportanceRange = Type::RegionImportanceRange<Importance> && std::ranges::forward_range<Importance>;

/**
 * `Importance` is a range of sparse importance matrix element.
 */
template<typename Importance>
concept SparseImportanceRange = SparseMatrixElement::ImportanceRange<Importance> && std::ranges::forward_range<Importance>;

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
concept KernelModifier =
	SparseMatrixElement::ImportanceRange<Importance>
	&& std::is_invocable_v<Modifier, std::add_lvalue_reference_t<Kernel>, std::ranges::range_const_reference_t<Importance>>;

//In-place modifies kernel with a modifier member function given a range of sparse importance matrix element.
template<
	typename Kernel,
	SparseImportanceRange Importance,
	KernelModifier<Kernel, Importance> Modifier
>
void modify(Kernel& kernel, Importance&& importance, Modifier modifier) {
	using std::ranges::cbegin, std::ranges::cend,
		std::for_each, std::execution::unseq,
		std::invoke,
		std::is_nothrow_invocable_v, std::add_lvalue_reference_t, std::ranges::range_const_reference_t;
	for_each(unseq, cbegin(importance), cend(importance),
		[&kernel, &modifier](const auto& element) noexcept(
			is_nothrow_invocable_v<Modifier, add_lvalue_reference_t<Kernel>, range_const_reference_t<Importance>>) {
			invoke(modifier, kernel, element);
		});
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

	ContainerType Importance_;

	//Modify one region by some amount.
	template<Internal_::DenseKernelBinaryOperator Op>
	void modify(SizeType, Op, ValueType) noexcept(std::is_nothrow_invocable_v<Op, ValueType, ValueType>);

	//Modify one region by a given sparse importance matrix element.
	template<Internal_::DenseKernelBinaryOperator Op>
	void modify(const SparseMatrixElement::Importance&, Op) noexcept(std::is_nothrow_move_constructible_v<Op>);

	//Modify all regions by some amount.
	template<Internal_::DenseKernelBinaryOperator Op, DenseImportanceRange Importance>
	void modify(Op op, Importance&& importance) {
		if constexpr (std::is_same_v<Op, std::minus<>>) {
			using std::ranges::all_of, std::views::zip;
			assert(all_of(zip(this->Importance_, importance), std::ranges::greater_equal {}));
		}
		Arithmetic::addRange(this->Importance_, std::move(op), std::forward<Importance>(importance), this->Importance_.begin());
	}

	//Modify some regions by some amount.
	template<SparseImportanceRange Importance>
	void modify(Importance&& importance, Internal_::KernelModifier<Dense&, Importance> auto modifier) {
		Internal_::modify(*this, std::forward<Importance>(importance), std::move(modifier));
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
	void resize(SizeType);

	/**
	 * @brief Clear all contents in the kernel and reset importance of all regions to zero.
	 */
	void clear() noexcept;

	/**
	 * @brief Increment the importance of a region by one.
	 *
	 * @param region_id Identifier of region whose importance is to be incremented.
	 */
	void increment(SizeType) noexcept;

	/**
	 * @brief Increment the importance of a region by some amount.
	 *
	 * @param importance A sparse importance matrix element used for incrementation.
	 */
	void increment(const SparseMatrixElement::Importance&) noexcept;

	/**
	 * @brief Increment the importance of all regions by some amount.
	 *
	 * @tparam Importance A range of importance for region at each index.
	 *
	 * @param importance The size of this range must be no less than the size of the kernel.
	 */
	template<DenseImportanceRange Importance>
	void increment(Importance&& importance) {
		this->modify(std::plus {}, std::forward<Importance>(importance));
	}

	/**
	 * @brief Increment the importance of some regions by some amount.
	 *
	 * @tparam Importance A range of sparse importance matrix element.
	 *
	 * @param importance Each specifies the region identifier and the amount of importance to increment.
	 */
	template<SparseImportanceRange Importance>
	void increment(Importance&& importance) {
		this->modify(std::forward<Importance>(importance),
			std::mem_fn(static_cast<void (Dense::*)(const SparseMatrixElement::Importance&)>(&Dense::increment)));
	}

	/**
	 * @brief Decrement the importance of a region by one.
	 *
	 * @param region_id Identifier of region whose importance is to be decremented.
	 */
	void decrement(SizeType) noexcept;

	/**
	 * @brief Decrement the importance of a region by some amount.
	 *
	 * @param importance A sparse importance matrix element used for decrementation.
	 */
	void decrement(const SparseMatrixElement::Importance&) noexcept;

	/**
	 * @brief Decrement the importance of all regions by some amount.
	 *
	 * @tparam Importance A range of importance for region at each index.
	 *
	 * @param importance The size of this range must be no less than the size of the kernel.
	 */
	template<DenseImportanceRange Importance>
	void decrement(Importance&& importance) {
		this->modify(std::minus {}, std::forward<Importance>(importance));
	}

	/**
	 * @brief Decrement the importance of some regions by some amount.
	 *
	 * @tparam Importance A range of sparse importance matrix element.
	 *
	 * @param importance Each specifies the region identifier and the amount of importance to decrement.
	 */
	template<SparseImportanceRange Importance>
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

	ValueContainerType Importance_;
	OffsetContainerType Offset;

	//Modify some regions by some amount.
	template<
		SparseImportanceRange Importance,
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
	void resize(SizeType);

	/**
	 * @brief Clear all contents in the kernel.
	 */
	void clear() noexcept;

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
	template<SparseImportanceRange Importance>
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
	template<SparseImportanceRange Importance>
	void decrement(Importance&& importance) {
		this->modify(
			std::forward<Importance>(importance), std::mem_fn(static_cast<void (Sparse::*)(const ValueType&)>(&Sparse::decrement)));
	}

};

}