#include <DisRegRep/Image/Serialisation/Buffer/Tile.hpp>

#include <DisRegRep/Core/View/Matrix.hpp>
#include <DisRegRep/Core/Bit.hpp>

#include <catch2/matchers/catch_matchers_range_equals.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include <glm/fwd.hpp>
#include <glm/vec2.hpp>

#include <array>
#include <span>

#include <functional>

#include <type_traits>

#include <cstdint>

namespace Bit = DisRegRep::Core::Bit;
using DisRegRep::Image::Serialisation::Buffer::Tile,
	DisRegRep::Core::View::Matrix::NewAxisLeft;

using Catch::Matchers::RangeEquals;

using std::to_array,
	std::span, std::as_bytes;
using std::bind_back, std::bit_or;
using std::remove_const_t;

namespace {

using ScalarType = std::uint_fast8_t;
using ValueType = ScalarType;
using ExtentType = glm::vec<2U, ScalarType>;

using TileType = Tile<ValueType>;

constexpr auto MatrixExtent = ExtentType(5U, 7U),
	TileExtent = ExtentType(4U, 3U),
	TileOffset = ExtentType(2U, 5U);

constexpr auto Matrix = to_array<ValueType>({
	4, 2, 6, 8, 7, 7, 7,
	8, 3, 0, 4, 9, 7, 8,
	5, 9, 2, 8, 6, 8, 4,
	2, 6, 3, 3, 3, 7, 4,
	3, 3, 0, 5, 9, 2, 3
});
constexpr auto TileContent = to_array<ValueType>({
	8, 4, 4,
	7, 4, 4,
	2, 3, 3,
	2, 3, 3
});
constexpr auto TileContentPacked = to_array<ValueType>({
	0x84, 0x44,
	0x74, 0x44,
	0x23, 0x33,
	0x23, 0x33
});
constexpr auto MatrixUntiled = to_array<ValueType>({
	0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 8, 4,
	0, 0, 0, 0, 0, 7, 4,
	0, 0, 0, 0, 0, 2, 3
});

constexpr Bit::BitPerSampleResult MatrixBitPerSampleResult = Bit::minimumBitPerSample(Matrix);

}

TEMPLATE_TEST_CASE_SIG("A multidimensional serialisation buffer with support for out-of-bound padding and bit packing", "[Image][Serialisation][Buffer][Tile]", ((bool Packed), Packed), false, true) {

	GIVEN("An tile buffer initialised for a matrix as data source") {
		static constexpr auto reshapeMatrix = bind_back(bit_or {}, NewAxisLeft(MatrixExtent.y));
		static constexpr auto ExpectedTileContent = [] static consteval noexcept {
			if constexpr (Packed) {
				return span(TileContentPacked);
			} else {
				return span(TileContent);
			}
		}();
		TileType tile;

		REQUIRE_NOTHROW(tile.allocate(ExpectedTileContent.size_bytes()));
		const span tile_buffer = tile.buffer();
		const auto shaped = tile.shape(
			[] static consteval noexcept {
				if constexpr (Packed) {
					return TileType::EnablePacking;
				} else {
					return TileType::DisablePacking;
				}
			}(), TileExtent, &MatrixBitPerSampleResult);

		WHEN("Tile is loaded from the matrix") {
			shaped.fromMatrix(reshapeMatrix(Matrix), TileOffset);

			THEN("Tile buffer is filled with content") {
				CHECK_THAT(tile_buffer, RangeEquals(as_bytes(ExpectedTileContent)));

				AND_WHEN("Tile is unloaded to an empty matrix") {
					remove_const_t<decltype(MatrixUntiled)> empty_matrix {};
					shaped.toMatrix(reshapeMatrix(empty_matrix), TileOffset);

					THEN("The empty matrix is filled with the exact same content as the original input with padding removed") {
						CHECK_THAT(empty_matrix, RangeEquals(MatrixUntiled));
					}

				}

			}

		}

	}

}