#pragma once

#include <DisRegRep/Core/View/Functional.hpp>

#include <glm/fwd.hpp>

#include <ranges>

#include <utility>

#include <concepts>

/**
 * @brief Generate a sequence of indices for iterating through a data structure for serialisation.
 */
namespace DisRegRep::Image::Serialisation::Index {

/**
 * @brief Divide a multidimensional matrix in tiles, and iterate through each tile by a coordinate containing the tile. Index
 * incrementation starts from the right-most rank.
 *
 * @tparam L Dimensionality.
 * @tparam MatrixExtent, TileExtent Matrix and tile extent type.
 *
 * @param matrix_extent Sizes of each rank of the matrix.
 * @param tile_extent Sizes of each rank of the tile. It must be in the same dimensionality as the matrix; consider padding the tile
 * extent with 1's if they differ.
 *
 * @return Tile index vector in `L` dimension.
 */
inline constexpr auto ForeachTile = []<
	glm::length_t L,
	std::integral MatrixExtent,
	std::integral TileExtent
>(const glm::vec<L, MatrixExtent> matrix_extent,
	const glm::vec<L, TileExtent> tile_extent) static constexpr noexcept -> std::ranges::view auto {
	using std::views::cartesian_product, std::views::iota, std::views::stride,
		std::integer_sequence, std::make_integer_sequence;
	return [&]<glm::length_t... I>(integer_sequence<glm::length_t, I...>) constexpr noexcept {
		return cartesian_product(iota(MatrixExtent {}, matrix_extent[I]) | stride(tile_extent[I])...);
	}(make_integer_sequence<glm::length_t, L> {})
		| Core::View::Functional::MakeFromTuple<glm::vec<L, MatrixExtent>>;
};

}