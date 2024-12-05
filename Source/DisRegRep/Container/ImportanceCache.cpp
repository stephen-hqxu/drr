#include <DisRegRep/Container/ImportanceCache.hpp>
#include <DisRegRep/Container/SparseMatrixElement.hpp>

#include <DisRegRep/Type.hpp>

#include <algorithm>
#include <execution>

#include <cassert>

using DisRegRep::Container::ImportanceCache::Sparse, DisRegRep::Container::SparseMatrixElement::Importance,
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

void Sparse::increment(const EntryType& importance) {
	const auto [region_id, value] = importance;
	if (OffsetType& offset = this->Offset[region_id];
		offset == Sparse::NoEntryOffset) {
		offset = this->Importance.size();
		this->Importance.push_back(importance);
	} else [[likely]] {
		this->Importance[region_id].Value += value;
	}
}

void Sparse::increment(const SizeType region_id) {
	this->increment(makeSingleImportance(region_id));
}

void Sparse::decrement(const EntryType& importance) {
	const auto [region_id, value] = importance;
	OffsetType& offset = this->Offset[region_id];
	assert(offset != Sparse::NoEntryOffset);

	if (const auto erasing_entry_it = this->Importance.begin() + offset;
		erasing_entry_it->Value <= value) {
		//Need to fully remove this cache entry and update offsets of all following entries.
		//i.e. maintaining sparsity.
		const auto following_entry_it = this->Importance.erase(erasing_entry_it);
		for_each(unseq, following_entry_it, this->Importance.end(),
			[&offset = this->Offset](const auto& entry) constexpr noexcept { --offset[entry.Identifier]; });

		offset = Sparse::NoEntryOffset;
	} else [[likely]] {
		erasing_entry_it->Value -= value;
	}
}

void Sparse::decrement(const SizeType region_id) {
	this->decrement(makeSingleImportance(region_id));
}