#include <DisRegRep/Container/SplatKernel.hpp>
#include <DisRegRep/Container/SparseMatrixElement.hpp>

#include <DisRegRep/Core/Type.hpp>

#include <span>
#include <tuple>

#include <algorithm>
#include <execution>
#include <functional>

#include <type_traits>
#include <utility>

#include <cassert>

namespace SpltKn = DisRegRep::Container::SplatKernel;
using SpltKn::Dense, SpltKn::Sparse, SpltKn::Internal_::DenseKernelBinaryOperator,
	DisRegRep::Container::SparseMatrixElement::Importance, DisRegRep::Core::Type::RegionIdentifier;

using std::span, std::tie, std::apply;
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
constexpr void Dense::modify(const IndexType region_id, Op op, const ValueType amount) noexcept(
	std::is_nothrow_invocable_v<Op, ValueType, ValueType>) {
	ValueType& importance = this->Importance_[region_id];
	if constexpr (is_same_v<Op, minus<>>) {
		assert(importance >= amount);
	}
	importance = invoke(std::move(op), importance, amount);
}

template<DenseKernelBinaryOperator Op>
constexpr void Dense::modify(const Importance& importance, Op op) noexcept(std::is_nothrow_move_constructible_v<Op>) {
	const auto [region_id, value] = importance;
	this->modify(region_id, std::move(op), value);
}

Dense::SizeType Dense::sizeByte() const noexcept {
	return ::span(this->Importance_).size_bytes();
}

void Dense::resize(const IndexType region_count) {
	this->Importance_.resize(region_count);
}

void Dense::clear() noexcept {
	fill(this->Importance_, ValueType {});
}

void Dense::increment(const IndexType region_id) noexcept {
	this->modify(region_id, plus {}, 1U);
}

void Dense::increment(const Importance& importance) noexcept {
	this->modify(importance, plus {});
}

void Dense::decrement(const IndexType region_id) noexcept {
	this->modify(region_id, minus {}, 1U);
}

void Dense::decrement(const Importance& importance) noexcept {
	this->modify(importance, minus {});
}

Sparse::SizeType Sparse::sizeByte() const noexcept {
	return apply([](const auto&... array) static constexpr noexcept { return (::span(array).size_bytes() + ...); },
		tie(this->Importance_, this->Offset));
}

void Sparse::resize(const IndexType region_count) {
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
		this->Importance_[offset].Value += value;
	}
}

void Sparse::increment(const IndexType region_id) {
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

void Sparse::decrement(const IndexType region_id) {
	this->decrement(makeSingleImportance(region_id));
}