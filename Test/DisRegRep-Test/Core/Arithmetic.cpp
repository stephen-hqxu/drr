#include <DisRegRep/Core/Arithmetic.hpp>

#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include <glm/fwd.hpp>
#include <glm/vector_relational.hpp>
#include <glm/vec2.hpp>

#include <array>

#include <string_view>

#include <algorithm>
#include <functional>
#include <ranges>

#include <cstdint>

namespace Arithmetic = DisRegRep::Core::Arithmetic;

using Catch::Matchers::WithinAbs;

using glm::u8vec2;

using std::array, std::string_view;
using std::ranges::transform, std::ranges::fold_left_first,
	std::plus,
	std::views::cartesian_product, std::views::iota, std::views::enumerate,
	std::ranges::range_value_t;

namespace {

namespace MeshGrid {

constexpr std::uint_fast8_t Width = 4U, Height = 3U;
constexpr auto Data = [] static consteval noexcept {
	array<u8vec2, 12U> mesh_grid {};
	transform(cartesian_product(iota(0U, Height), iota(0U, Width)), mesh_grid.begin(), [](const auto it) static consteval noexcept {
		const auto [y, x] = it;
		return u8vec2(x, y);
	});
	return mesh_grid;
}();

}

enum class View2dCategory : std::uint_fast8_t {
	Normal = 0x00U,
	Transposed = 0x80U,
	SubView = 0xFFU
};

}

SCENARIO("Normalise: Divide a range of numeric values by a factor", "[Core][Arithmetic]") {

	GIVEN("A range of values") {
		const auto size = GENERATE(take(3U, random(5U, 20U)));
		const auto number = GENERATE_COPY(take(1U, chunk(size, random(-100, 100))));

		WHEN("Values are normalised by their sum") {
			const auto sum = 1.0F * fold_left_first(number, plus {}).value_or(1);
			const auto normalised_number = number | Arithmetic::Normalise(sum);

			THEN("Sum of normalised values equals one") {
				const auto normalised_sum = fold_left_first(normalised_number, plus {});
				REQUIRE(normalised_sum);
				CHECK_THAT(*normalised_sum, WithinAbs(1.0F, 1e-4F));
			}

		}

	}

}

TEMPLATE_TEST_CASE_SIG("View2d/ViewTransposed2d/SubRange2d: View a linear range as a 2D matrix, or transposed, or as a sub-matrix", "[Core][Arithmetic]",
	((View2dCategory Cat), Cat), View2dCategory::Normal, View2dCategory::Transposed, View2dCategory::SubView) {
	using enum View2dCategory;

	GIVEN("A flattened 2D matrix whose elements are their corresponding coordinates") {
		static constexpr auto ViewText = [] {
			using namespace std::string_view_literals;
			switch (Cat) {
			case Normal: return "2D"sv;
			case Transposed: return "transposed 2D"sv;
			case SubView: return "sub-2D"sv;
			}
		}();

		WHEN("A " << ViewText << " view is formed") {
			static constexpr auto Offset = u8vec2(2U, 1U), Extent = u8vec2(1U, 2U);

			static constexpr auto MeshGrid2d = MeshGrid::Data | Arithmetic::View2d(MeshGrid::Width);
			static constexpr auto MeshGrid2dT = MeshGrid::Data | Arithmetic::ViewTransposed2d(MeshGrid::Width);
			static constexpr auto SubMeshGrid2d = MeshGrid2d | Arithmetic::SubRange2d(Offset, Extent);

			static constexpr auto CheckIdx = [](const auto& rg, const u8vec2 offset = {}) static -> void {
				for (const auto [y, outer] : rg | enumerate) [[likely]] {
					for (const auto [x, inner] : outer | enumerate) [[likely]] {
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
				}
			}

		}

	}

}