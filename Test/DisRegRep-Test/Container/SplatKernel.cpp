#include <DisRegRep/Container/SplatKernel.hpp>

#include <DisRegRep/Container/SparseMatrixElement.hpp>

#include <DisRegRep/Core/Type.hpp>

#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>

#include <catch2/matchers/catch_matchers_container_properties.hpp>
#include <catch2/matchers/catch_matchers_predicate.hpp>
#include <catch2/matchers/catch_matchers_quantifiers.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>

#include <catch2/catch_test_macros.hpp>

#include <span>

#include <algorithm>
#include <functional>
#include <ranges>

#include <concepts>

namespace SpMatElem = DisRegRep::Container::SparseMatrixElement;
namespace Type = DisRegRep::Core::Type;
using DisRegRep::Container::SplatKernel::Dense, DisRegRep::Container::SplatKernel::Sparse,
	SpMatElem::Importance;

using Catch::Matchers::SizeIs, Catch::Matchers::IsEmpty,
	Catch::Matchers::AllMatch, Catch::Matchers::Predicate, Catch::Matchers::RangeEquals;

using std::span;
using std::ranges::fill,
	std::bind_front,
	std::ranges::equal_to, std::plus, std::multiplies,
	std::views::drop, std::views::repeat, std::views::as_const,
	std::views::transform, std::views::zip_transform, std::views::stride,
	std::ranges::input_range;
using std::unsigned_integral;

namespace {

[[nodiscard]] auto generateDenseImportance(const unsigned_integral auto size) {
	return GENERATE_COPY(take(3U, chunk(size * 2UZ, random<Type::RegionImportance>(500U, 8500U)))) | std::views::chunk(size);
}

//for all a' in a, b' in b : a' + 2b'
[[nodiscard]] constexpr auto sumDouble(const input_range auto& a, const input_range auto& b) noexcept {
	return zip_transform(plus {}, a, b | transform(bind_front(multiplies {}, 2U)));
}

}

SCENARIO("Dense: A dense array of region importance where index corresponds to region identifier", "[Container][SplatKernel]") {
	static constexpr Dense::IndexType RegionCount = 20U;
	const auto match_no_importance =
		AllMatch(Predicate<Dense::ValueType>(bind_front(equal_to {}, Dense::ValueType {}), "Region importance equals zero"));

	GIVEN("A default initialised dense splat kernel") {
		Dense kernel;

		THEN("Kernel is empty") {
			REQUIRE_THAT(kernel, SizeIs(0U));
			REQUIRE_THAT(kernel, IsEmpty());
		}

		WHEN("Kernel is resized") {
			kernel.resize(RegionCount);
			const span kernel_span = kernel.span();

			THEN("Kernel has the correct size") {
				REQUIRE_THAT(kernel, SizeIs(RegionCount));
				REQUIRE_THAT(kernel, !IsEmpty());
			}

			THEN("Region importances are all zeros") {
				REQUIRE_THAT(kernel_span, match_no_importance);
			}

			AND_GIVEN("Some region importances") {
				static constexpr auto Importance = repeat(Dense::ValueType { 7'316'129 }, RegionCount);

				WHEN("Kernel is incremented with these and subsequently cleared") {
					kernel.increment(Importance);
					kernel.clear();

					THEN("Kernel size remains unchanged") {
						CHECK_THAT(kernel, SizeIs(RegionCount));
					}

					THEN("Region importances are reset to zeros") {
						CHECK_THAT(kernel_span, match_no_importance);
					}

				}

			}

		}

	}

	GIVEN("An empty dense splat kernel with memory allocated") {
		Dense kernel;
		kernel.resize(RegionCount);
		const span kernel_span = kernel.span();

		const auto then_kernel_is_cleared = [kernel_span, &match_no_importance]() -> void {
			THEN("Kernel is cleared") {
				CHECK_THAT(kernel_span, match_no_importance);
			}
		};

		WHEN("Incremented with region identifiers") {
			const auto rg = GENERATE(take(5U, random<Dense::IndexType>(1U, RegionCount - 1U)));
			kernel.increment(0U);
			kernel.increment(rg);
			kernel.increment(rg);

			THEN("Importance of such regions is incremented by one per incrementation") {
				CHECK(kernel_span[0U] == 1U);
				CHECK(kernel_span[rg] == 2U);
			}

			THEN("Importance of other regions remains zero") {
				using std::views::take;
				CHECK_THAT(kernel_span | take(rg) | drop(1U), match_no_importance);
				CHECK_THAT(kernel_span | drop(rg + 1U), match_no_importance);
			}

			AND_WHEN("Decremented with the same region identifiers") {
				kernel.decrement(0U);
				kernel.decrement(rg);
				kernel.decrement(rg);

				then_kernel_is_cleared();
			}

		}

		WHEN("Incremented with sparse matrix elements") {
			const auto element_data = GENERATE(take(5U, chunk(3U, random<Dense::IndexType>(1U, RegionCount - 1U))));
			const Importance element0 {
				.Identifier = 0U,
				.Value = element_data[0]
			}, element1 {
				.Identifier = element_data[1],
				.Value = element_data[2] * Dense::ValueType { 10000 }
			};
			kernel.increment(element0);
			kernel.increment(element1);
			kernel.increment(element1);

			const auto [rg0, value0] = element0;
			const auto [rg1, value1] = element1;

			THEN("Importance of such regions is incremented by their element values per incrementation") {
				CHECK(kernel_span[rg0] == value0);
				CHECK(kernel_span[rg1] == value1 * 2U);
			}

			THEN("Importance of other regions remains zero") {
				using std::views::take;
				CHECK_THAT(kernel_span | take(rg1) | drop(1U), match_no_importance);
				CHECK_THAT(kernel_span | drop(rg1 + 1U), match_no_importance);
			}

			AND_WHEN("Decremented with the same sparse matrix elements") {
				kernel.decrement(element0);
				kernel.decrement(element1);
				kernel.decrement(element1);

				then_kernel_is_cleared();
			}

		}

		WHEN("Incremented with dense region importance ranges") {
			const auto dense = generateDenseImportance(RegionCount);
			kernel.increment(dense[0]);
			kernel.increment(dense[1]);
			kernel.increment(dense[1]);

			THEN("Importance of all regions is incremented by values in the dense range") {
				CHECK_THAT(kernel_span, RangeEquals(sumDouble(dense[0], dense[1])));
			}

			AND_WHEN("Decremented with the same dense region importance ranges") {
				kernel.decrement(dense[0]);
				kernel.decrement(dense[1]);
				kernel.decrement(dense[1]);

				then_kernel_is_cleared();
			}

		}

		WHEN("Incremented with sparse region importance ranges") {
			auto dense = generateDenseImportance(RegionCount);
			fill(dense[0] | stride(2U), Dense::ValueType {});
			fill(dense[1] | stride(3U), Dense::ValueType {});
			auto sparse0 = dense[0] | as_const | SpMatElem::ToSparse,
				sparse1 = dense[1] | as_const | SpMatElem::ToSparse;

			kernel.increment(sparse0);
			kernel.increment(sparse1);
			kernel.increment(sparse1);

			THEN("Importance of regions presented in the sparse range is incremented by the corresponding values") {
				CHECK_THAT(kernel_span, RangeEquals(sumDouble(dense[0], dense[1])));
			}

			AND_WHEN("Decremented with the same sparse region importance ranges") {
				kernel.decrement(sparse0);
				kernel.decrement(sparse1);
				kernel.decrement(sparse1);

				then_kernel_is_cleared();
			}

		}

	}

}