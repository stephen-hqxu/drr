#pragma once

#include "../Bit.hpp"

#include <DisRegRep/Core/View/Arithmetic.hpp>
#include <DisRegRep/Core/View/Matrix.hpp>
#include <DisRegRep/Core/UninitialisedAllocator.hpp>

#include <glm/fwd.hpp>

#include <span>
#include <tuple>
#include <vector>

#include <algorithm>
#include <execution>
#include <iterator>
#include <ranges>

#include <bit>
#include <utility>

#include <concepts>
#include <type_traits>

#include <cassert>

namespace DisRegRep::Image::Serialisation::Buffer {

namespace TileInternal_ {

/**
 * Input range that can be used as a copy source in a parallelisable algorithm.
 */
template<typename In>
concept SimpleLinearCopyableInput = std::ranges::forward_range<In> && std::ranges::common_range<In>;
/**
 * Output range that can be written from an input range in a parallelisable algorithm.
 */
template<typename Out, typename In>
concept SimpleLinearCopyableOutput = std::ranges::output_range<Out, std::ranges::range_const_reference_t<In>>
	&& std::ranges::forward_range<Out> && std::indirectly_copyable<std::ranges::const_iterator_t<In>, std::ranges::iterator_t<Out>>;

}

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

	//Tags for deducing packing argument when creating a shaped tile view.
	static constexpr std::bool_constant<true> EnablePacking;
	static constexpr std::bool_constant<false> DisablePacking;

private:

	template<glm::length_t Rank>
	using RankConstant = std::integral_constant<glm::length_t, Rank>;

public:

	/**
	 * @brief Tile with an arbitrary dimensionality.
	 *
	 * @tparam Const If tile buffer is read-only.
	 * @tparam Packed Data stored in the tile buffer is bit-packed.
	 * @tparam W Multidimensional view to the linear tile buffer.
	 * @tparam Ext Extent type.
	 */
	template<bool Const, bool Packed, std::ranges::input_range W, std::integral Ext>
	requires std::ranges::view<W> && std::ranges::sized_range<W> && std::is_move_constructible_v<W>
	class [[nodiscard]] Shaped {
	public:

		friend Tile;

		static constexpr bool IsConstant = Const, RequirePacking = Packed;
		using ViewType = W;
		using ExtentType = Ext;

	private:

		ViewType View;
		/**
		 * @brief Additional specification required when bits packing is required. In additional to @link Bit::BitPerSampleResult, it
		 * also includes a size of the right-most axis of the tile without any bit packing consideration.
		 */
		std::conditional_t<RequirePacking,
			std::tuple<const Bit::BitPerSampleResult*, ExtentType>,
			std::tuple<>
		> PackSpec;

		explicit constexpr Shaped(ViewType view) noexcept(std::is_nothrow_move_constructible_v<ViewType>) : View(std::move(view)) { }

		explicit constexpr Shaped(ViewType view, const Bit::BitPerSampleResult& bps_result, const ExtentType last_rank_extent)
			noexcept(std::is_nothrow_move_constructible_v<ViewType>)
		requires(RequirePacking)
			: View(std::move(view)), PackSpec(&bps_result, last_rank_extent) { }

		//Get the current tile extent.
		template<bool FinalRank, std::ranges::sized_range R>
		[[nodiscard]] constexpr ExtentType currentExtent(R&& r) const
			noexcept(FinalRank || std::is_nothrow_invocable_v<decltype(std::ranges::size), R>)
		requires(RequirePacking) {
			if constexpr (FinalRank) {
				return std::get<1U>(this->PackSpec);
			} else {
				return std::ranges::size(std::forward<R>(r));
			}
		}
		template<bool, std::ranges::sized_range R>
		[[nodiscard]] static constexpr ExtentType currentExtent(R&& r)
			noexcept(std::is_nothrow_invocable_v<decltype(std::ranges::size), R>)
		requires(!RequirePacking) {
			return std::ranges::size(std::forward<R>(r));
		}

		//Copy every element from the input range to the output range.
		template<TileInternal_::SimpleLinearCopyableInput In, TileInternal_::SimpleLinearCopyableOutput<In> Out>
		static void copySimple(In&& in, Out&& out) {
			using std::copy, std::execution::unseq,
				std::ranges::cbegin, std::ranges::cend, std::ranges::begin;
			copy(unseq, cbegin(in), cend(in), begin(std::forward<Out>(out)));
		}

		//Pack elements from the input range and write the result to the output range.
		template<Core::View::Arithmetic::ClampToEdgePaddableRange<ExtentType> In,
			std::ranges::output_range<std::ranges::range_const_reference_t<In>> Out>
		requires std::ranges::viewable_range<In> && std::ranges::common_range<In>
			&& std::ranges::forward_range<Out> && std::ranges::sized_range<Out>
		void copyPacked(In&& in, Out&& out) const
		requires(RequirePacking) {
			using std::transform, std::execution::unseq,
				std::views::chunk,
				std::ranges::size, std::ranges::begin;
			const Bit::BitPerSampleResult& bps_result = *std::get<0U>(this->PackSpec);

			const auto in_packed = std::forward<In>(in) | chunk(bps_result.PackingFactor);
			assert(size(in_packed) == size(out));
			auto out_it = begin(std::forward<Out>(out));

			transform(unseq, in_packed.cbegin(), in_packed.cend(), std::move(out_it),
				[&bps_result]<typename PackGroup>(PackGroup pack_group) constexpr noexcept(
					std::is_nothrow_move_constructible_v<PackGroup>) { return Bit::pack(std::move(pack_group), bps_result); });
		}
		//A fallback function when packing is not required.
		template<TileInternal_::SimpleLinearCopyableInput In, TileInternal_::SimpleLinearCopyableOutput<In> Out>
		static void copyPacked(In&& in, Out&& out)
		requires(!RequirePacking) {
			Shaped::copySimple(std::forward<In>(in), std::forward<Out>(out));
		}

		//Unpack elements from the input range a write the result to the output range.
		template<std::ranges::forward_range In, std::ranges::forward_range Out>
		requires std::ranges::viewable_range<In> && std::ranges::viewable_range<Out> && std::ranges::sized_range<Out>
			&& std::indirectly_copyable<std::ranges::const_iterator_t<In>, std::ranges::iterator_t<Out>>
		void copyUnpacked(In&& in, Out&& out) const
		requires(RequirePacking) {
			using std::for_each, std::execution::unseq,
				std::ranges::copy,
				std::views::zip, std::views::chunk;
			const Bit::BitPerSampleResult& bps_result = *std::get<0U>(this->PackSpec);

			const auto in_out_pair = zip(std::forward<In>(in), std::forward<Out>(out) | chunk(bps_result.PackingFactor));
			for_each(unseq, in_out_pair.cbegin(), in_out_pair.cend(), [&bps_result](const auto in_out) constexpr noexcept {
				const auto [in_packed, out_group] = in_out;
				copy(Bit::unpack(in_packed, out_group.size(), bps_result), out_group.begin());
			});
		}

		//A fallback function when unpacking is not required.
		template<TileInternal_::SimpleLinearCopyableInput In, TileInternal_::SimpleLinearCopyableOutput<In> Out>
		static void copyUnpacked(In&& in, Out&& out)
		requires(!RequirePacking) {
			Shaped::copySimple(std::forward<In>(in), std::forward<Out>(out));
		}

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
			using std::views::drop, std::views::take, std::views::zip,
				std::ranges::input_range, std::ranges::viewable_range,
				std::ranges::range_const_reference_t, std::ranges::range_reference_t;
			[this, &tile_offset]<
				glm::length_t Rank,
				Core::View::Arithmetic::ClampToEdgePaddableRange<ExtentType> In,
				input_range Out,
				bool FinalRank = Rank == L - 1
			> requires viewable_range<In> && viewable_range<Out>
				&& (FinalRank
					|| (std::is_move_constructible_v<range_const_reference_t<In>> && std::is_move_constructible_v<range_reference_t<Out>>))
			(this const auto& self, RankConstant<Rank>, In&& in, Out&& out) constexpr {
				const ExtentType current_extent = this->template currentExtent<FinalRank>(out);
				auto in_padded = std::forward<In>(in)
					| drop(tile_offset[Rank])
					| take(current_extent)
					| Core::View::Arithmetic::PadClampToEdge(current_extent);
				if constexpr (FinalRank) {
					this->copyPacked(std::move(in_padded), std::forward<Out>(out));
				} else {
					for (auto [tile_in, tile_out] : zip(std::move(in_padded), std::forward<Out>(out))) [[likely]] {
						self(RankConstant<Rank + 1> {}, std::move(tile_in), std::move(tile_out));
					}
				}
			}(RankConstant<0> {}, std::forward<Matrix>(matrix), this->View);
		}

		/**
		 * @brief Copy contents from the tile buffer to a multidimensional matrix with the same number of rank.
		 *
		 * @tparam Matrix Matrix type.
		 * @tparam L Dimensionality.
		 * @tparam Q Type qualifier.
		 *
		 * @param matrix Provide copy destination.
		 * @param tile_offset Coordinate of the first element of the tile in `matrix`.
		 */
		template<typename Matrix, glm::length_t L, glm::qualifier Q>
		constexpr void toMatrix(Matrix&& matrix, const glm::vec<L, ExtentType, Q>& tile_offset) const {
			using std::views::drop, std::views::take, std::views::zip,
				std::ranges::input_range, std::ranges::viewable_range, std::ranges::sized_range,
				std::ranges::range_const_reference_t, std::ranges::range_reference_t;
			[this, &tile_offset]<
				glm::length_t Rank,
				input_range In,
				input_range Out,
				bool FinalRank = Rank == L - 1
			> requires viewable_range<In> && viewable_range<Out> && sized_range<Out>
				&& (FinalRank
					|| (std::is_move_constructible_v<range_const_reference_t<In>> && std::is_move_constructible_v<range_reference_t<Out>>))
			(this const auto& self, RankConstant<Rank>, In&& in, Out&& out) constexpr {
				auto in_trimmed = std::forward<In>(in)
					| drop(tile_offset[Rank])
					| take(size(out));
				if constexpr (FinalRank) {
					this->copyUnpacked(std::move(in_trimmed), std::forward<Out>(out));
				} else {
					for (auto [tile_in, tile_out] : zip(std::move(in_trimmed), std::forward<Out>(out))) [[likely]] {
						self(RankConstant<Rank + 1> {}, std::move(tile_in), std::move(tile_out));
					}
				}
			}(RankConstant<0> {}, this->View, std::forward<Matrix>(matrix));
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
	 * @tparam Packed Should bits packing be enabled.
	 * @tparam L Dimensionality.
	 * @tparam Ext Extent type.
	 * @tparam Q Type qualifier.
	 *
	 * @param tile_extent Size of each rank of the tile.
	 * @param bps_result If provided, it specifies how bits from a target buffer are packed into the tile. A valid pointer must be
	 * provided if bits packing is enabled. This memory must survive beyond the constructed @link Shaped object.
	 *
	 * @return @link Shaped.
	 */
	template<typename Self, bool Packed, glm::length_t L, std::integral Ext, glm::qualifier Q>
	constexpr auto shape(this Self& self, std::bool_constant<Packed>, const glm::vec<L, Ext, Q>& tile_extent,
		const Bit::BitPerSampleResult* const bps_result = nullptr) noexcept {
		if constexpr (Packed) {
			assert(bps_result);
		}
		auto shaped_memory = [&tile_extent, bps_result]<glm::length_t Rank, typename Matrix>(
			this const auto& self,
			RankConstant<Rank>,
			Matrix&& matrix
		) constexpr noexcept -> std::ranges::view auto {
			if constexpr (Rank == 0) {
				return std::forward<Matrix>(matrix) | std::views::all;
			} else {
				Ext extent = tile_extent[Rank];
				if constexpr (Packed && Rank == L - 1) {
					const auto [_1, packing_factor, packing_factor_log2, _2] = *bps_result;
					extent = (extent + packing_factor - 1U) >> packing_factor_log2;
				}
				return self(RankConstant<Rank - 1> {}, std::forward<Matrix>(matrix) | Core::View::Matrix::NewAxisLeft(extent));
			}
		}(RankConstant<L - 1> {}, self.Memory);

		//Don't bother writing a deduction guide, it is just too much typing to write a constructor for every member.
		if constexpr (using ShapedType = Shaped<std::is_const_v<Self>, Packed, decltype(shaped_memory), Ext>;
			Packed) {
			return ShapedType(std::move(shaped_memory), *bps_result, tile_extent[L - 1]);
		} else {
			return ShapedType(std::move(shaped_memory));
		}
		
		
	}

};

}