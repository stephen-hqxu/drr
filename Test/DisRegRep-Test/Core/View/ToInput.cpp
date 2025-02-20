#include <DisRegRep/Core/View/ToInput.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <functional>
#include <ranges>

using DisRegRep::Core::View::ToInput;

using std::ranges::equal,
	std::bind_front, std::multiplies;
using std::views::iota, std::views::transform,
	std::ranges::input_range, std::ranges::forward_range,
	std::ranges::bidirectional_range, std::ranges::random_access_range, std::ranges::common_range,
	std::ranges::sized_range;

SCENARIO("Resulting range is always an input range", "[Core][View][ToInput]") {

	GIVEN("A range of numbers that is not just an input range") {
		static constexpr auto EvenNumber = iota(0U, 200U) | transform(bind_front(multiplies {}, 2U));

		using EvenNumberType = decltype(EvenNumber);
		STATIC_REQUIRE(
			input_range<EvenNumberType>
			&& forward_range<EvenNumberType>
			&& bidirectional_range<EvenNumberType>
			&& random_access_range<EvenNumberType>
			&& common_range<EvenNumberType>
		);
		STATIC_REQUIRE(sized_range<EvenNumberType>);

		WHEN("Such range is piped into the to input view") {
			static constexpr auto EvenNumberToInput = EvenNumber | ToInput;
			using EvenNumberInputType = decltype(EvenNumberToInput);

			THEN("It is an input range and nothing more") {
				STATIC_CHECK(
					input_range<EvenNumberInputType> && !(
						forward_range<EvenNumberInputType>
						|| bidirectional_range<EvenNumberInputType>
						|| random_access_range<EvenNumberInputType>
						|| common_range<EvenNumberInputType>
					)
				);
			}

			THEN("It is still a sized range") {
				STATIC_CHECK(sized_range<EvenNumberInputType>);
				STATIC_CHECK(EvenNumberToInput.size() == EvenNumber.size());
			}

			THEN("Its value equals the original range") {
				STATIC_CHECK(equal(EvenNumberToInput, EvenNumber));
			}

		}

	}

}