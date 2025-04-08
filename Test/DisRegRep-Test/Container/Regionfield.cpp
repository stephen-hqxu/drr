#include <DisRegRep/Container/Regionfield.hpp>

#include <DisRegRep/RegionfieldGenerator/ExecutionPolicy.hpp>
#include <DisRegRep/RegionfieldGenerator/Uniform.hpp>

#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <catch2/matchers/catch_matchers_container_properties.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <catch2/catch_get_random_seed.hpp>
#include <catch2/catch_test_macros.hpp>

#include <glm/gtc/type_ptr.hpp>

#include <span>

#include <algorithm>
#include <functional>
#include <ranges>

#include <cstdint>

namespace RfGenExec = DisRegRep::RegionfieldGenerator::ExecutionPolicy;
using DisRegRep::Container::Regionfield, DisRegRep::RegionfieldGenerator::Uniform;

using Catch::Matchers::IsEmpty, Catch::Matchers::SizeIs,
	Catch::Matchers::ContainsSubstring, Catch::Matchers::RangeEquals;

using glm::make_vec2, glm::value_ptr;

using std::span;
using std::ranges::fold_left_first,
	std::multiplies,
	std::views::join;

SCENARIO("Regionfield is a matrix of region identifiers", "[Container][Regionfield]") {

	GIVEN("A default constructed regionfield") {
		Regionfield rf;

		THEN("Its size is zero") {
			REQUIRE_THAT(rf, SizeIs(0U));
			REQUIRE_THAT(rf, IsEmpty());
		}

		WHEN("Resized") {
			const Regionfield::DimensionType dim =
				make_vec2(GENERATE(take(3U, chunk(2U, random<std::uint_least8_t>(5U, 15U)))).data());
			REQUIRE_NOTHROW(rf.resize(dim));

			AND_WHEN("There is at least one of the extent component being zero") {

				THEN("Matrix cannot be resized") {
					CHECK_THROWS_WITH(
						rf.resize(Regionfield::DimensionType(0U, dim.y)), ContainsSubstring("greaterThan") && ContainsSubstring("0U"));
				}

			}

			THEN("Linear size of regionfield container is the product of each extent component") {
				REQUIRE_THAT(rf, SizeIs(*fold_left_first(span(value_ptr(dim), Regionfield::DimensionType::length()), multiplies {})));
				REQUIRE_THAT(rf, !IsEmpty());
			}

			THEN("Dimension of regionfield matrix equals extent") {
				REQUIRE(rf.extent() == dim);
			}

		}

		AND_GIVEN("A regionfield generator") {
			static constexpr Uniform Generator;

			THEN("Regionfield can be filled with region identifiers") {
				rf.resize(make_vec2(GENERATE(take(2U, chunk(2U, random<std::uint_least8_t>(5U, 20U)))).data()));
				rf.RegionCount = GENERATE(take(2U, random<Regionfield::ValueType>(1U, 10U)));
				Generator(RfGenExec::MultiThreadingTrait, rf, {
					.Seed = Catch::getSeed()
				});

				WHEN("Matrix is transposed") {
					const auto rf_t = rf.transpose();

					THEN("Informative fields are unchanged") {
						CHECK(rf_t.RegionCount == rf.RegionCount);
					}

					THEN("Total size remains the same") {
						CHECK_THAT(rf_t, SizeIs(rf.size()));
					}

					THEN("Its extent gets swapped") {
						const Regionfield::DimensionType ext = rf.extent(),
							ext_t = rf_t.extent();

						CHECK(ext_t.x == ext.y);
						CHECK(ext_t.y == ext.x);
					}

					THEN("Data are transposed") {
						auto rf_view_t = rf.rangeTransposed2d() | join;
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