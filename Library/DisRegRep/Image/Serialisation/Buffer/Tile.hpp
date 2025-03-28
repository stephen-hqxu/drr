#pragma once

#include "../Bit.hpp"

#include <DisRegRep/Core/View/Arithmetic.hpp>
#include <DisRegRep/Core/View/Matrix.hpp>
#include <DisRegRep/Core/UninitialisedAllocator.hpp>

#include <glm/fwd.hpp>

#include <span>
#include <vector>

#include <algorithm>
#include <execution>
#include <ranges>

#include <bit>
#include <utility>

#include <concepts>
#include <type_traits>

#include <cassert>

namespace DisRegRep::Image::Serialisation::Buffer {

/**
 * @brief Buffer for reading from and writing to an image that supports tile-based memory access.
 *
 * @tparam V Image element type.
 */
template<typename V>
requires std::is_arithmetic_v<V>
class [[nodiscard]] Tile {
public:

	using ValueType = V;
	using MemoryType = std::vector<ValueType, Core::UninitialisedAllocator<ValueType>>;
	using SizeType = typename MemoryType::size_type;

private:

	template<glm::length_t Rank>
	using RankConstant = std::integral_constant<glm::length_t, Rank>;

public:

	/**
	 * @brief Tile with an arbitrary dimensionality.
	 *
	 * @tparam Const If tile buffer is read-only.
	 * @tparam W Multidimensional view to the linear tile buffer.
	 * @tparam Ext Extent type.
	 */
	template<bool Const, std::ranges::input_range W, std::integral Ext>
	requires std::ranges::view<W> && std::ranges::sized_range<W> && std::is_move_constructible_v<W>
	class [[nodiscard]] Shaped {
	public:

		friend Tile;

		static constexpr bool IsConstant = Const;
		using ViewType = W;
		using ExtentType = Ext;

	private:

		const Bit::BitPerSampleResult* PackSpec;
		ViewType View;
		//Size of the right-most axis of the tile without any bit packing consideration.
		ExtentType LastRankExtent;

		constexpr Shaped(const Bit::BitPerSampleResult* const pack_spec, ViewType view, const ExtentType last_rank_extent)
			noexcept(std::is_nothrow_move_constructible_v<ViewType>) :
			PackSpec(pack_spec), View(std::move(view)), LastRankExtent(last_rank_extent) { }

	public:

		/**
		 * @brief Copy contents from a multidimensional matrix with the same number of rank to the tile buffer.
		 *
		 * @tparam Matrix Matrix type.
		 * @tparam L Dimensionality.
		 * @tparam Q Type qualifier.
		 *
		 * @param matrix Provide data source to be copied from.
		 * @param tile_offset Coordinate of the first element of the tile in `matrix`.
		 */
		template<typename Matrix, glm::length_t L, glm::qualifier Q>
		constexpr void fromMatrix(Matrix&& matrix, const glm::vec<L, ExtentType, Q>& tile_offset) const
		requires(!IsConstant) {
			//Parallel may not be beneficial because we are copying line by line.
			using std::transform, std::copy, std::execution::unseq,
				std::views::zip, std::views::drop, std::views::take, std::views::chunk,
				std::ranges::size, std::ranges::begin,
				std::ranges::range_const_reference_t, std::ranges::input_range, std::ranges::output_range, std::ranges::viewable_range;
			[
				pack_spec = this->PackSpec,
				last_rank_extent = this->LastRankExtent,
				&tile_offset
			]<
				glm::length_t Rank,
				Core::View::Arithmetic::ClampToEdgePaddableRange<ExtentType> In,
				typename Out,
				typename InRef = range_const_reference_t<In>,
				bool FinalRank = Rank == L - 1
			> requires viewable_range<In>
				&& ((!FinalRank && std::is_move_constructible_v<InRef>)
					|| (FinalRank && output_range<Out, InRef>))
			(this const auto& self, RankConstant<Rank>, In&& in, Out&& out) constexpr {
				const ExtentType out_size = size(out),
					current_extent = FinalRank ? last_rank_extent : out_size;
				const auto in_padded = std::forward<In>(in)
					| drop(tile_offset[Rank])
					| take(current_extent)
					| Core::View::Arithmetic::PadClampToEdge(current_extent);
				if constexpr (FinalRank) {
					auto out_it = begin(std::forward<Out>(out));
					if (!pack_spec) {
						copy(unseq, in_padded.cbegin(), in_padded.cend(), std::move(out_it));
						return;
					}
					const auto in_packed = in_padded | chunk(pack_spec->PackingFactor);
					assert(size(in_packed) == out_size);
					transform(unseq, in_packed.cbegin(), in_packed.cend(), std::move(out_it),
						[pack_spec](auto pack_group) constexpr noexcept { return Bit::pack(std::move(pack_group), *pack_spec); });
				} else {
					for (auto [tile_in, tile_out] : zip(in_padded, std::forward<Out>(out))) [[likely]] {
						self(RankConstant<Rank + 1> {}, std::move(tile_in), std::move(tile_out));
					}
				}
			}(RankConstant<0> {}, std::forward<Matrix>(matrix), this->View);
		}

	};

private:

	MemoryType Memory;

public:

	constexpr Tile() noexcept = default;

	Tile(const Tile&) = delete;

	constexpr Tile(Tile&&) noexcept = default;

	Tile& operator=(const Tile&) = delete;

	constexpr Tile& operator=(Tile&&) noexcept = default;

	constexpr ~Tile() = default;

	/**
	 * @brief Allocate tile buffer.
	 *
	 * @param byte Size in bytes.
	 */
	constexpr void allocate(const SizeType byte) {
		this->Memory.resize(byte >> std::countr_zero(sizeof(ValueType)));
	}

	/**
	 * @brief Get the underlying buffer memory.
	 *
	 * @return Byte array pointed to the buffer memory.
	 */
	[[nodiscard]] constexpr auto buffer() noexcept {
		return std::as_writable_bytes(std::span(this->Memory));
	}

	/**
	 * @brief View the linear tile buffer as a multidimensional matrix.
	 *
	 * @tparam L Dimensionality.
	 * @tparam Ext Extent type.
	 * @tparam Q Type qualifier.
	 *
	 * @param tile_extent Size of each rank of the tile.
	 * @param pack_spec If provided, it specifies how bits from a target buffer are packed into the tile.
	 *
	 * @return @link Shaped.
	 */
	template<typename Self, glm::length_t L, std::integral Ext, glm::qualifier Q>
	constexpr auto shape(
		this Self& self,
		const glm::vec<L, Ext, Q>& tile_extent,
		const Bit::BitPerSampleResult* const pack_spec = nullptr
	) noexcept {
		auto shaped_memory = [
			packing_factor_log2 = pack_spec ? pack_spec->PackingFactorLog2 : Bit::BitPerSampleResult::BitType {},
			&tile_extent
		]<glm::length_t Rank, typename Matrix>(
			this const auto& self,
			RankConstant<Rank>,
			Matrix&& matrix
		) constexpr noexcept -> std::ranges::view auto {
			if constexpr (Rank == 0) {
				return std::forward<Matrix>(matrix) | std::views::all;
			} else {
				Ext extent = tile_extent[Rank];
				if constexpr (Rank == L - 1) {
					extent >>= packing_factor_log2;
				}
				return self(RankConstant<Rank - 1> {}, std::forward<Matrix>(matrix) | Core::View::Matrix::NewAxisLeft(extent));
			}
		}(RankConstant<L - 1> {}, self.Memory);
		//Don't bother writing a deduction guide, it is just too much typing to write a constructor for every member.
		return Shaped<std::is_const_v<Self>, decltype(shaped_memory), Ext>(pack_spec, std::move(shaped_memory), tile_extent[L - 1]);
	}

};

}