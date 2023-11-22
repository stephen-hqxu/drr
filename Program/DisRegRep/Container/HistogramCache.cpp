#include <DisRegRep/Container/HistogramCache.hpp>

#include <cassert>

using std::for_each;

using namespace DisRegRep;
using Format::Bin_t, Format::Region_t;
using HistogramCache::Dense, HistogramCache::Sparse;

namespace {

constexpr Sparse::bin_type createSingleValueBin(const Region_t region) noexcept {
	return {
		.Region = region,
		.Value = 1u
	};
}

}

void Sparse::increment(const bin_type& bin) {
	const auto [region, value] = bin;
	if (index_type& region_idx = this->SparseSet[region];
		region_idx == Sparse::NoEntryIndex) {
		region_idx = static_cast<index_type>(this->DenseSet.size());
		this->DenseSet.push_back(bin);
	} else {
		this->DenseSet[region_idx].Value += value;
	}
}

void Sparse::increment(const Region_t region) {
	return this->increment(::createSingleValueBin(region));
}

void Sparse::decrement(const bin_type& bin) {
	const auto [region, value] = bin;
	index_type& region_idx = this->SparseSet[region];
	assert(region_idx != Sparse::NoEntryIndex);

	const auto removing_bin_it = this->DenseSet.begin() + region_idx;
	if (Bin_t& removing_value = removing_bin_it->Value;
		removing_value <= value) {
		//need to fully remove this bin and update indices of all following entries
		const auto following_bin_it = this->DenseSet.erase(removing_bin_it);
		for_each(following_bin_it, this->DenseSet.end(), [&sparse = this->SparseSet](const auto& bin) constexpr noexcept {
			sparse[bin.Region]--;
		});

		region_idx = Sparse::NoEntryIndex;
	} else {
		removing_value -= value;
	}
}

void Sparse::decrement(const Region_t region) {
	return this->decrement(::createSingleValueBin(region));
}