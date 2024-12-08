#include <DisRegRep/Container/CombineKernel.hpp>
#include <DisRegRep/Container/SparseMatrixElement.hpp>

#include <DisRegRep/Type.hpp>

#include <algorithm>
#include <execution>

#include <cassert>

using DisRegRep::Container::CombineKernel::Sparse, DisRegRep::Container::SparseMatrixElement::Importance,
	DisRegRep::Type::RegionIdentifier;

using std::for_each, std::execution::unseq;

namespace {

[[nodiscard]] constexpr Importance makeSingleImportance(const RegionIdentifier region_id) noexcept {
	return {
		.Identifier = region_id,
		.Value = 1U
	};
}

}

void Sparse::increment(const ValueType& importance) {
	const auto [region_id, value] = importance;
	if (OffsetType& offset = this->Offset[region_id];
		offset == Sparse::NoValueOffset) {
		offset = this->Importance.size();
		this->Importance.push_back(importance);
	} else [[likely]] {
		this->Importance[region_id].Value += value;
	}
}

void Sparse::increment(const SizeType region_id) {
	this->increment(makeSingleImportance(region_id));
}

void Sparse::decrement(const ValueType& importance) {
	const auto [region_id, value] = importance;
	OffsetType& offset = this->Offset[region_id];
	assert(offset != Sparse::NoValueOffset);

	if (const auto erasing_value_it = this->Importance.begin() + offset;
		erasing_value_it->Value <= value) {
		//Need to fully remove this value from the array and update offsets of all following entries.
		//i.e. maintaining sparsity.
		const auto following_value_it = this->Importance.erase(erasing_value_it);
		for_each(unseq, following_value_it, this->Importance.end(),
			[&offset = this->Offset](const auto& entry) constexpr noexcept { --offset[entry.Identifier]; });

		offset = Sparse::NoValueOffset;
	} else [[likely]] {
		erasing_value_it->Value -= value;
	}
}

void Sparse::decrement(const SizeType region_id) {
	this->decrement(makeSingleImportance(region_id));
}