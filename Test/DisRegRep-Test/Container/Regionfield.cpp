#include <DisRegRep/Container/Regionfield.hpp>

#include <DisRegRep/Core/Arithmetic.hpp>

#include <DisRegRep/RegionfieldGenerator/Uniform.hpp>

#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <catch2/matchers/catch_matchers_container_properties.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <functional>
#include <ranges>

#include <utility>

#include <cstddef>
#include <cstdint>

namespace Arithmetic = DisRegRep::Core::Arithmetic;
using DisRegRep::Container::Regionfield, DisRegRep::RegionfieldGenerator::Uniform;

using Catch::Matchers::IsEmpty, Catch::Matchers::SizeIs,
	Catch::Matchers::ContainsSubstring, Catch::Matchers::RangeEquals;

using std::ranges::fold_left_first,
	std::multiplies,
	std::views::join;
using std::index_sequence, std::make_index_sequence;

SCENARIO("Regionfield is a matrix of region identifiers", "[Container][Regionfield]") {

	GIVEN("A default constructed regionfield") {
		Regionfield rf;

		THEN("Its size is zero") {
			REQUIRE_THAT(rf, SizeIs(0U));
			REQUIRE_THAT(rf, IsEmpty());
		}

		WHEN("Resized") {
			const auto dim = GENERATE(take(3U, chunk(2U, random<std::uint_least8_t>(5U, 15U))));
			REQUIRE_NOTHROW(rf.resize(Regionfield::DimensionType(dim[0], dim[1])));

			AND_WHEN("There is at least one of the extent component being zero") {

				THEN("Matrix cannot be resized") {
					CHECK_THROWS_WITH(rf.resize(Regionfield::DimensionType(0U, dim[1])),
						ContainsSubstring("greaterThan") && ContainsSubstring("0U"));
				}

			}

			THEN("Linear size of regionfield container is the product of each extent component") {
				const auto rf_span = rf.span();
				REQUIRE_THAT(rf_span, SizeIs(*fold_left_first(dim, multiplies {})));
				REQUIRE_THAT(rf_span, !IsEmpty());
			}

			THEN("Dimension of regionfield matrix equals extent") {
				[&extent = rf.mapping().extents(), &dim]<std::size_t... I>(index_sequence<I...>) {
					REQUIRE(((extent.extent(I) == dim[I]) && ...));
				}(make_index_sequence<Regionfield::MdSpanType::rank()> {});
			}

		}

		AND_GIVEN("A regionfield generator") {
			static constexpr Uniform Generator;

			THEN("Regionfield can be filled with region identifiers") {
				const auto dim = GENERATE(take(2U, chunk(2U, random<std::uint_least8_t>(5U, 20U))));
				rf.resize(Regionfield::DimensionType(dim[0], dim[1]));
				rf.RegionCount = GENERATE(take(2U, random<Regionfield::ValueType>(1U, 10U)));
				Generator(rf);

				WHEN("Matrix is transposed") {
					const auto rf_t = rf.transpose();

					THEN("Informative fields are unchanged") {
						CHECK(rf_t.RegionCount == rf.RegionCount);
					}

					THEN("Its extent gets swapped") {
						const Regionfield::ExtentType& ext = rf.mapping().extents(),
							&ext_t = rf_t.mapping().extents();

						CHECK(ext_t.extent(0U) == ext.extent(1U));
						CHECK(ext_t.extent(1U) == ext.extent(0U));
					}

					THEN("Data are transposed") {
						auto rf_view_t = rf.span() | Arithmetic::ViewTransposed2d(rf.mapping().stride(1U)) | join;
						CHECK_THAT(rf_t.range2d() | join, RangeEquals(rf_view_t));
					}

					AND_WHEN("Matrix is transposed again") {
						const auto rf_t_t = rf_t.transpose();

						THEN("It is identical to its original state") {
							CHECK(rf_t_t == rf);
						}

					}

				}

			}

		}

	}

}