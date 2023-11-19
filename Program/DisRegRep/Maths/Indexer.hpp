#pragma once

#include <array>
#include <algorithm>
#include <ranges>

#include <utility>
#include <concepts>

#include <cassert>

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
private:

	constexpr static size_t AxisCount = sizeof...(Prec);
	constexpr static std::array AxisPrecedence = { Prec... },
		//This array basically tells which axis has the highest precedence, and so on.
		AxisOrder = []() consteval noexcept -> auto {
			std::array<size_t, AxisCount> order { };
			for (const auto axis : std::views::iota(0u, AxisCount)) {
				order[AxisPrecedence[axis]] = axis;
			}
			return order;
		}();
	
	//Stride of each axis based on precedence.
	std::array<size_t, AxisCount> Extent, CumSumExtent;

	//Calculate extent cumulative sum.
	template<typename T>
	constexpr static T calcExtentCumulativeSum(const T& extent) {
		T cum_sum_ext { };

		size_t cum_ext = 1u;
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
	 * @tparam TExt Type of extent.
	 * @param extent The array containing extent length.
	*/
	constexpr Indexer(const std::array<size_t, AxisCount>& extent) :
		Extent(extent), CumSumExtent(Indexer::calcExtentCumulativeSum(this->Extent)) {
		assert(std::ranges::all_of(extent, [](const auto ext) constexpr noexcept { return std::cmp_greater(ext, 0); }));
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
	constexpr Indexer(const TExt... extent) : Indexer(std::array { static_cast<size_t>(extent)... }) {
		
	}

	constexpr ~Indexer() = default;

	/**
	 * @brief Get the extent length for an axis.
	 * 
	 * @param axis The axis index.
	 * 
	 * @return The axis extent.
	*/
	constexpr size_t extent(const size_t axis) const noexcept {
		return this->Extent[axis];
	}

	/**
	 * @brief Calculate linear index given coordinate for each axis.
	 * 
	 * @tparam TIdx... Type of index.
	 * @param idx... Index into each axis.
	*/
	template<std::integral... TIdx>
	requires(sizeof...(TIdx) == AxisCount)
	constexpr size_t operator()(const TIdx... idx) const noexcept {
		using std::index_sequence;
		constexpr static auto axis_idx = std::make_index_sequence<AxisCount> { };

		//extent range check
		assert((std::cmp_greater_equal(idx, 0) && ...));
		assert(([&extent = this->Extent, idx...]<size_t... Idx>(index_sequence<Idx...>) {
			return (std::cmp_less(idx, extent[Idx]) && ...);
		}(axis_idx)));

		const auto calc = [&stride = this->CumSumExtent, idx...]<size_t... Idx>(index_sequence<Idx...>) noexcept -> size_t {
			return ((idx * stride[Idx]) + ...);
		};
		return calc(axis_idx);
	}

};

}