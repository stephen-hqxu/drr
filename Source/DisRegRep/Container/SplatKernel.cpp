#include <DisRegRep/Container/SplatKernel.hpp>
#include <DisRegRep/Container/SparseMatrixElement.hpp>

#include <DisRegRep/Type.hpp>

#include <algorithm>
#include <execution>
#include <functional>

#include <type_traits>
#include <utility>

#include <cassert>

namespace SplatKernel = DisRegRep::Container::SplatKernel;
using SplatKernel::Dense, SplatKernel::Sparse, SplatKernel::Internal_::DenseKernelBinaryOperator,
	DisRegRep::Container::SparseMatrixElement::Importance, DisRegRep::Type::RegionIdentifier;

using std::for_each, std::execution::unseq,
	std::ranges::fill;
using std::plus, std::minus, std::invoke;
using std::is_same_v;

namespace {

[[nodiscard]] constexpr Importance makeSingleImportance(const RegionIdentifier region_id) noexcept {
	return {
		.Identifier = region_id,
		.Value = 1U
	};
}

}

template<DenseKernelBinaryOperator Op>
void Dense::modify(const SizeType region_id, Op op, const ValueType amount) noexcept(
	std::is_nothrow_invocable_v<Op, ValueType, ValueType>) {
	ValueType& importance = this->Importance_[region_id];
	if constexpr (is_same_v<Op, minus<>>) {
		assert(importance >= amount);
	}
	importance = invoke(std::move(op), importance, amount);
}

template<DenseKernelBinaryOperator Op>
void Dense::modify(const Importance& importance, Op op) noexcept(std::is_nothrow_move_constructible_v<Op>) {
	const auto [region_id, value] = importance;
	this->modify(region_id, std::move(op), value);
}

void Dense::resize(const SizeType region_count) {
	this->Importance_.resize(region_count);
}

void Dense::clear() noexcept {
	fill(this->Importance_, ValueType {});
}

void Dense::increment(const SizeType region_id) noexcept {
	this->modify(region_id, plus {}, 1U);
}

void Dense::increment(const Importance& importance) noexcept {
	this->modify(importance, plus {});
}

void Dense::decrement(const SizeType region_id) noexcept {
	this->modify(region_id, minus {}, 1U);
}

void Dense::decrement(const Importance& importance) noexcept {
	this->modify(importance, minus {});
}

void Sparse::resize(const SizeType region_count) {
	this->Offset.resize(region_count, Sparse::NoValueOffset);
}

void Sparse::clear() noexcept {
	this->Importance_.clear();
	fill(this->Offset, Sparse::NoValueOffset);
}

void Sparse::increment(const ValueType& importance) {
	const auto [region_id, value] = importance;
	if (OffsetType& offset = this->Offset[region_id];
		offset == Sparse::NoValueOffset) {
		offset = this->Importance_.size();
		this->Importance_.push_back(importance);
	} else [[likely]] {
		this->Importance_[region_id].Value += value;
	}
}

void Sparse::increment(const SizeType region_id) {
	this->increment(makeSingleImportance(region_id));
}

void Sparse::decrement(const ValueType& importance) {
	const auto [region_id, value] = importance;
	OffsetType& offset = this->Offset[region_id];
	assert(offset != Sparse::NoValueOffset);

	if (const auto erasing_value_it = this->Importance_.begin() + offset;
		erasing_value_it->Value <= value) {
		//Need to fully remove this value from the array and update offsets of all following entries.
		//i.e. maintaining sparsity.
		const auto following_value_it = this->Importance_.erase(erasing_value_it);
		for_each(unseq, following_value_it, this->Importance_.end(),
			[&offset = this->Offset](const auto& entry) constexpr noexcept { --offset[entry.Identifier]; });

		offset = Sparse::NoValueOffset;
	} else [[likely]] {
		erasing_value_it->Value -= value;
	}
}

void Sparse::decrement(const SizeType region_id) {
	this->decrement(makeSingleImportance(region_id));
}