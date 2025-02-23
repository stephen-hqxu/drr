#include <DisRegRep/Core/View/Matrix.hpp>
#include <DisRegRep/Core/View/Functional.hpp>

#include <DisRegRep-Test/StringMaker.hpp>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include <glm/fwd.hpp>
#include <glm/vector_relational.hpp>
#include <glm/vec2.hpp>

#include <array>
#include <string_view>

#include <algorithm>
#include <ranges>

#include <utility>

#include <cstdint>

namespace Matrix = DisRegRep::Core::View::Matrix;

using glm::u8vec2;

using std::array;
using std::ranges::copy,
	std::views::cartesian_product, std::views::iota, std::views::enumerate;

namespace {

namespace MeshGrid {

constexpr std::uint_fast8_t Width = 4U, Height = 3U;
constexpr auto Data = [] static consteval noexcept {
	using DisRegRep::Core::View::Functional::MakeFromTuple;
	array<u8vec2, 12U> mesh_grid {};
	copy(cartesian_product(
		iota(std::uint_fast8_t {}, Width),
		iota(std::uint_fast8_t {}, Height)
	) | MakeFromTuple<u8vec2>, mesh_grid.begin());
	return mesh_grid;
}();

}

enum class View2dCategory : std::uint_fast8_t {
	Normal = 0x00U,
	Transposed = 0x80U,
	SubView = 0xFFU
};

}

TEMPLATE_TEST_CASE_SIG("View2d/ViewTransposed2d/SubRange2d: View a linear range as a 2D matrix, or transposed, or as a sub-matrix", "[Core][View][Matrix]",
	((View2dCategory Cat), Cat), View2dCategory::Normal, View2dCategory::Transposed, View2dCategory::SubView) {
	using enum View2dCategory;

	GIVEN("A flattened 2D matrix whose elements are their corresponding coordinates") {
		static constexpr auto ViewText = [] static consteval noexcept {
			using namespace std::string_view_literals;
			switch (Cat) {
			case Normal: return "2D"sv;
			case Transposed: return "transposed 2D"sv;
			case SubView: return "sub-2D"sv;
			default:
				std::unreachable();
			}
		}();

		WHEN("A " << ViewText << " view is formed") {
			static constexpr auto Offset = u8vec2(2U, 1U), Extent = u8vec2(1U, 2U);

			static constexpr auto MeshGrid2d = MeshGrid::Data | Matrix::View2d(MeshGrid::Height);
			static constexpr auto MeshGrid2dT = MeshGrid::Data | Matrix::ViewTransposed2d(MeshGrid::Height);
			static constexpr auto SubMeshGrid2d = MeshGrid2d | Matrix::SubRange2d(Offset, Extent);

			//NOLINTNEXTLINE(readability-function-cognitive-complexity)
			static constexpr auto CheckIdx = [](const auto& rg, const u8vec2 offset = {}) static -> void {
				for (const auto [x, outer] : rg | enumerate) [[likely]] {
					for (const auto [y, inner] : outer | enumerate) [[likely]] {
						u8vec2 coordinate;
						if constexpr (Cat == Transposed) {
							coordinate = u8vec2(y, x);
						} else {
							coordinate = u8vec2(x, y);
						}
						if constexpr (Cat == SubView) {
							CHECK(glm::all(glm::lessThan(coordinate, Extent)));
						}
						coordinate += offset;
						CHECK(coordinate == inner);
					}
				}
			};

			THEN("Its coordinate equals to its element") {
				switch (Cat) {
				case Normal: CheckIdx(MeshGrid2d);
					break;
				case Transposed: CheckIdx(MeshGrid2dT);
					break;
				case SubView: CheckIdx(SubMeshGrid2d, Offset);
					break;
				default:
					std::unreachable();
				}
			}

		}

	}

}