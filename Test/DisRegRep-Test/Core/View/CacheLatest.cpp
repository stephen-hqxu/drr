#include <DisRegRep/Core/View/CacheLatest.hpp>

#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <functional>
#include <ranges>

#include <cstdint>

using DisRegRep::Core::View::CacheLatest;

using std::ranges::fold_left_first,
	std::plus,
	std::views::iota, std::views::transform, std::views::filter;

SCENARIO("Latest dereferenced result from a range iterator is cached to avoid repetitive call", "[Core][View][CacheLatest]") {

	GIVEN("An cached expensive range") {
		const auto size = GENERATE(take(3U, random<std::uint_fast8_t>(40U, 100U)));
		std::uint_fast8_t deref_counter {};
		auto expensive_rg = iota(0U, size) | transform([&deref_counter](const auto i) constexpr noexcept {
			++deref_counter;
			return i;
		}) | CacheLatest;

		WHEN("Range is consumed when the iterator is dereferenced multiple times per iteration") {
			auto multi_deref_rg = expensive_rg | filter([](const auto i) static constexpr noexcept { return (i & 1U) == 0U; });
			[[maybe_unused]] const volatile auto sum = fold_left_first(multi_deref_rg, plus {});

			THEN("The expensive range is only evaluated once per iteration regardless of how many times it is dereferenced") {
				CHECK(deref_counter == size);
			}

		}

	}

}