#include <DisRegRep/Core/View/Arithmetic.hpp>

#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>

#include <catch2/matchers/catch_matchers_container_properties.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/matchers/catch_matchers_quantifiers.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>

#include <catch2/catch_test_macros.hpp>

#include <glm/fwd.hpp>

#include <algorithm>
#include <functional>
#include <ranges>

#include <cstdint>

namespace Arithmetic = DisRegRep::Core::View::Arithmetic;

using Catch::Matchers::IsEmpty,
	Catch::Matchers::WithinAbs, Catch::Matchers::RangeEquals, Catch::Matchers::AllMatch;

using std::ranges::fold_left_first, std::ranges::sort,
	std::plus, std::minus, std::negate, std::bind_front,
	std::views::single, std::views::repeat, std::views::pairwise_transform, std::views::transform;

SCENARIO("Normalise: Divide a range of numeric values by a factor", "[Core][View][Arithmetic]") {

	GIVEN("A range of values") {
		const auto size = GENERATE(take(3U, random<std::uint_fast8_t>(5U, 20U)));
		const auto number = GENERATE_COPY(take(1U, chunk(size, random<std::int_least8_t>(-100, 100))));

		WHEN("Values are normalised by their sum") {
			const auto sum = 1.0F * *fold_left_first(number, plus {});
			const auto normalised_number = number | Arithmetic::Normalise(sum);

			THEN("Sum of normalised values equals one") {
				const auto normalised_sum = *fold_left_first(normalised_number, plus {});
				CHECK_THAT(normalised_sum, WithinAbs(1.0F, 1e-4F));
			}

		}

	}

}

SCENARIO("LinSpace: Create a range of evenly space numbers over a specified interval", "[Core][View][Arithmetic]") {

	GIVEN("An interval") {
		auto interval = GENERATE(take(3U, chunk(2U, random<glm::float32_t>(-10.0F, 10.0F))));
		sort(interval);
		const auto from = interval.front(), to = interval.back();
		const auto linspace_n = bind_front(Arithmetic::LinSpace, from, to);

		WHEN("Size is zero") {

			THEN("LinSpace is empty") {
				CHECK_THAT(linspace_n(0U), IsEmpty());
			}

		}

		WHEN("Size is one") {

			THEN("LinSpace has a single value of the starting interval") {
				CHECK_THAT(linspace_n(1U), RangeEquals(single(from)));
			}

		}

		WHEN("Size is greater than one") {
			const auto size = GENERATE(take(1U, random<std::uint_fast8_t>(2U, 20U)));
			const auto linspace = linspace_n(size);

			THEN("First and last value equals the interval bound") {
				CHECK_THAT(linspace.front(), WithinAbs(from, 1e-4F));
				CHECK_THAT(linspace.back(), WithinAbs(to, 1e-4F));
			}

			THEN("Values are linearly distributed") {
				const auto adjacent_difference = linspace | pairwise_transform(minus {}) | transform(negate {});

				CHECK_THAT(adjacent_difference, AllMatch(WithinAbs(adjacent_difference.front(), 1e-4F)));
			}

		}

	}

}

SCENARIO("PadClampToEdge: Pad a range up to the specified size by repeating the last element", "[Core][View][Arithmetic]") {

	GIVEN("An array of values") {
		const auto size = GENERATE(take(3U, random<std::uint_fast8_t>(10U, 25U)));
		const auto value = GENERATE_COPY(take(1U, chunk(size, random<std::uint_least8_t>(0U, 100U))));
		const auto make_padded = [&value](const auto padding_size) constexpr noexcept -> auto {
			return value | Arithmetic::PadClampToEdge(padding_size);
		};

		WHEN("Padding size is smaller than the array") {
			const auto padded = make_padded(size / 2U);

			THEN("No padding is done; the original array is unmodified") {
				CHECK_THAT(padded, RangeEquals(value));
			}

		}

		WHEN("Padding size is greater than the array") {
			const auto padded = make_padded(size * 2U);

			THEN("The padded range is clamped to edge") {
				using std::views::take, std::views::drop;
				const auto last = value.back();

				CHECK_THAT(padded | take(size), RangeEquals(value));
				CHECK_THAT(padded | drop(size), RangeEquals(repeat(last, size)));
			}

		}

	}

}