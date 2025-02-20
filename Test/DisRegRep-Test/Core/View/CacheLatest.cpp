#include <DisRegRep/Core/View/CacheLatest.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <functional>
#include <ranges>

#include <utility>

#include <cstdint>

using DisRegRep::Core::View::CacheLatest;

using std::ranges::fold_left_first,
	std::bind_front, std::plus, std::bit_and,
	std::views::iota, std::views::transform, std::views::filter;

SCENARIO("Latest dereferenced result from a range iterator is cached to avoid repetitive call", "[Core][View][CacheLatest]") {

	GIVEN("An expensive range") {
		static constexpr std::uint_fast8_t Size = 100U;
		static constexpr auto evalMultiDerefRange = []<typename R>(R&& r) static consteval noexcept -> void {
			auto multi_deref_rg = std::forward<R>(r) | filter(bind_front(bit_and {}, 1U));
			[[maybe_unused]] const volatile auto _ = fold_left_first(multi_deref_rg, plus {});
		};
		static constexpr auto evalExpensiveRange = [](const bool cache_latest) static consteval noexcept -> auto {
			std::uint_fast8_t deref_counter {};
			auto expensive_rg = iota(std::uint_fast8_t {}, Size) | transform([&deref_counter](const auto i) consteval noexcept {
				++deref_counter;
				return i;
			});

			if (cache_latest) {
				evalMultiDerefRange(expensive_rg | CacheLatest);
			} else {
				evalMultiDerefRange(expensive_rg);
			}
			return deref_counter;
		};

		WHEN("Range is consumed and the iterator is dereferenced multiple times per iteration") {

			THEN("It is evaluated more than once per iteration") {
				STATIC_CHECK(evalExpensiveRange(false) > Size);
			}

			AND_WHEN("Hold on! Let's cache the range before performing such consumption") {

				THEN("The cached expensive range is only evaluated once per iteration regardless of how many times it is dereferenced") {
					STATIC_CHECK(evalExpensiveRange(true) == Size);
				}

			}

		}

	}

}