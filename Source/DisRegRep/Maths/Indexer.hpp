#pragma once

#include <array>
#include <algorithm>
#include <ranges>

#include <utility>
#include <concepts>

#include <cassert>
#include <cstddef>
#include <cstdint>

namespace DisRegRep {

/**
 * @brief A helper class to facilitate indexing of contiguous memory in multi-dimension array.
 * 
 * @tparam Prec... Specify precedence for x, y, z, w, ... axis, respectively.
 * Axis with a higher precedence gets better linearity, thus cache locality.
*/
template<size_t... Prec>
requires(sizeof...(Prec) > 0u)
class Indexer {
public:

	using index_type = std::size_t;
	using rank_type = std::uint8_t;

private:

	constexpr static rank_type AxisCount = sizeof...(Prec);
	constexpr static std::array AxisPrecedence = { Prec... },
		//This array basically tells which axis has the highest precedence, and so on.
		AxisOrder = []() consteval static noexcept {
			std::array<rank_type, AxisCount> order { };
			for (const auto axis : std::views::iota(0u, AxisCount)) {
				order[AxisPrecedence[axis]] = axis;
			}
			return order;
		}();
	
	//Stride of each axis based on precedence.
	std::array<index_type, AxisCount> Extent, CumSumExtent;

	//Calculate extent cumulative sum.
	template<typename T>
	constexpr static T calcExtentCumulativeSum(const T& extent) {
		T cum_sum_ext { };

		index_type cum_ext = 1u;
		cum_sum_ext[AxisOrder.front()] = 1u;
		for (const auto [prev, curr] : AxisOrder | std::views::adjacent<2u>) {
			cum_ext *= extent[prev];
			cum_sum_ext[curr] = cum_ext;
		}
		return cum_sum_ext;
	}

public:

	constexpr Indexer() noexcept : Extent { }, CumSumExtent { } {

	}

	/**
	 * @brief Initialise indexer with array of extent length.
	 * 
	 * @tparam R The type of range of extent.
	 * @param extent The range containing extent lengths.
	*/
	template<std::ranges::input_range R>
	requires std::convertible_to<std::ranges::range_value_t<R>, index_type>
	constexpr Indexer(R&& extent) {
		std::ranges::copy(std::forward<R>(extent), this->Extent.begin());
		this->CumSumExtent = Indexer::calcExtentCumulativeSum(this->Extent);

		assert(std::ranges::all_of(extent, [](const auto ext) constexpr static noexcept { return std::cmp_greater(ext, 0); }));
	}

	/**
	 * @brief Initialise indexer with extent length for each axis: x, y, z, w, ...
	 * The behaviour is undefined if any extent is not positive.
	 * 
	 * @tparam TExt... Type of extent length.
	 * @param extent... Extent length. Must be strictly positive.
	*/
	template<std::integral... TExt>
	requires(sizeof...(TExt) == AxisCount)
	constexpr Indexer(const TExt... extent) : Indexer(std::array { static_cast<index_type>(extent)... }) {
		
	}

	constexpr ~Indexer() = default;

	/**
	 * @brief Get the extent length for an axis.
	 * 
	 * @param axis The axis index.
	 * If not provided, return the array containing extents for all axes.
	 * 
	 * @return The axis extent.
	*/
	constexpr index_type extent(const rank_type axis) const noexcept {
		return this->Extent[axis];
	}
	constexpr const auto& extent() const noexcept {
		return this->Extent;
	}

	/**
	 * @brief Calculate linear index given coordinate for each axis.
	 * 
	 * @tparam TIdx... Type of index.
	 * @param idx... Index into each axis.
	*/
	template<std::integral... TIdx>
	requires(sizeof...(TIdx) == AxisCount)
	constexpr index_type operator[](const TIdx... idx) const noexcept {
		using std::index_sequence;
		constexpr static auto AxisIdx = std::make_index_sequence<AxisCount> {};

		//extent range check
		assert((std::cmp_greater_equal(idx, 0) && ...));
		assert(([&extent = this->Extent, idx...]<index_type... Idx>(index_sequence<Idx...>) {
			return (std::cmp_less(idx, extent[Idx]) && ...);
		}(AxisIdx)));

		const auto calc = [&stride = this->CumSumExtent, idx...]<index_type... Idx>(index_sequence<Idx...>) noexcept -> index_type {
			return ((idx * stride[Idx]) + ...);
		};
		return calc(AxisIdx);
	}

};

}