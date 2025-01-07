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

#include <array>
#include <span>

#include <algorithm>
#include <functional>
#include <ranges>

#include <utility>

#include <type_traits>

namespace SpltKn = DisRegRep::Container::SplatKernel;
namespace SpMatElem = DisRegRep::Container::SparseMatrixElement;
namespace Type = DisRegRep::Core::Type;
using SpltKn::Dense, SpltKn::Sparse,
	SpMatElem::Importance;

using Catch::Matchers::SizeIs, Catch::Matchers::IsEmpty,
	Catch::Matchers::AllMatch, Catch::Matchers::Predicate, Catch::Matchers::RangeEquals;

using std::to_array, std::span;
using std::ranges::fill,
	std::bind_front,
	std::ranges::equal_to, std::plus, std::multiplies,
	std::views::drop, std::views::transform, std::views::zip_transform, std::views::stride,
	std::ranges::input_range, std::ranges::sized_range;
using std::common_type_t;

namespace {

using RegionCountType = common_type_t<Dense::IndexType, Sparse::IndexType>;
constexpr RegionCountType RegionCount = 20U;

template<typename A, typename B>
void incrementKernel(SpltKn::Is auto& kernel, A&& a, B&& b) {
	kernel.increment(std::forward<A>(a));
	kernel.increment(std::forward<B>(b));
	kernel.increment(std::forward<B>(b));
}

template<typename A, typename B>
void decrementKernel(SpltKn::Is auto& kernel, A&& a, B&& b) {
	kernel.decrement(std::forward<A>(a));
	kernel.decrement(std::forward<B>(b));
	kernel.decrement(std::forward<B>(b));
}

[[nodiscard]] auto matchNoImportance() {
	return AllMatch(Predicate<Dense::ValueType>(bind_front(equal_to {}, Dense::ValueType {}), "Region importance equals zero"));
}

[[nodiscard]] auto generatePositiveRegionIdentifier() {
	return GENERATE(take(5U, random<RegionCountType>(1U, RegionCount - 1U)));
}

[[nodiscard]] auto generateSparseMatrixElement() {
	const auto element_data = GENERATE(take(5U, chunk(3U, random<RegionCountType>(1U, RegionCount - 1U))));
	return to_array<Importance>({
		{ 0U, element_data[0] },
		{ element_data[1], element_data[2] * Importance::ValueType { 10000 } }
	});
}

[[nodiscard]] auto generateDenseImportance() {
	return GENERATE_COPY(take(3U, chunk(RegionCount * 2UZ, random<Type::RegionImportance>(500U, 8500U))))
		 | std::views::chunk(RegionCount);
}

[[nodiscard]] auto generateSparseImportance() {
	auto dense = generateDenseImportance();
	fill(dense[0] | stride(2U), Dense::ValueType {});
	fill(dense[1] | stride(3U), Dense::ValueType {});
	return dense;
}

//for all a' in a, b' in b : a' + 2b'
[[nodiscard]] constexpr auto sumDouble(const input_range auto& a, const input_range auto& b) noexcept {
	return zip_transform(plus {}, a, b | transform(bind_front(multiplies {}, 2U)));
}

}

SCENARIO("Dense: Well-defined container property behaviours", "[Container][SplatKernel]") {
	const auto match_no_importance = matchNoImportance();

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
			}

			THEN("Region importances are all zeros") {
				REQUIRE_THAT(kernel_span, match_no_importance);
			}

			AND_GIVEN("Some region importances") {
				auto rg = GENERATE(take(3U, chunk(RegionCount, random<Dense::ValueType>(10U, 550U))));

				WHEN("Kernel is incremented and decremented with these") {

					THEN("Content changes are predictable: incrementation and decrementation modifies importance") {
						const auto* const kernel_data = kernel_span.data();
						kernel.increment(rg);
						CHECK_THAT(kernel_span, RangeEquals(rg));

						static constexpr Dense::IndexType DecrementingRegion = 13U;
						kernel.decrement(Importance {
							.Identifier = DecrementingRegion,
							.Value = 2U
						});
						rg[DecrementingRegion] -= 2U;
						CHECK_THAT(kernel_span, RangeEquals(rg));

						AND_WHEN("Kernel is cleared") {
							kernel.clear();

							THEN("Kernel size remains unchanged") {
								CHECK_THAT(kernel, SizeIs(RegionCount));

								AND_THEN("Iterators remain valid") {
									REQUIRE(kernel_span.data() == kernel_data);
								}

							}

							THEN("Region importances are reset to zeros") {
								CHECK_THAT(kernel_span, match_no_importance);
							}

						}

					}

				}

			}

		}

	}

}

SCENARIO("Dense: A dense array of region importance where index corresponds to region identifier", "[Container][SplatKernel]") {
	const auto match_no_importance = matchNoImportance();

	GIVEN("An empty dense splat kernel with memory allocated") {
		Dense kernel;
		kernel.resize(RegionCount);
		const span kernel_span = kernel.span();

		const auto then_kernel_is_cleared = [&kernel_span, &match_no_importance] -> void {
			THEN("Kernel is cleared") {
				CHECK_THAT(kernel_span, match_no_importance);
			}
		};

		WHEN("Incremented with region identifiers") {
			const auto rg = generatePositiveRegionIdentifier();
			incrementKernel(kernel, 0U, rg);

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
				decrementKernel(kernel, 0U, rg);
				then_kernel_is_cleared();
			}

		}

		WHEN("Incremented with sparse matrix elements") {
			const auto [elem0, elem1] = generateSparseMatrixElement();
			incrementKernel(kernel, elem0, elem1);

			const auto [rg0, value0] = elem0;
			const auto [rg1, value1] = elem1;

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
				decrementKernel(kernel, elem0, elem1);
				then_kernel_is_cleared();
			}

		}

		WHEN("Incremented with dense region importance ranges") {
			const auto dense = generateDenseImportance();
			incrementKernel(kernel, dense[0], dense[1]);

			THEN("Importance of all regions is incremented by values in the dense range") {
				CHECK_THAT(kernel_span, RangeEquals(sumDouble(dense[0], dense[1])));
			}

			AND_WHEN("Decremented with the same dense region importance ranges") {
				decrementKernel(kernel, dense[0], dense[1]);
				then_kernel_is_cleared();
			}

		}

		WHEN("Incremented with sparse region importance ranges") {
			const auto sparse = generateSparseImportance();
			auto sparse0 = sparse[0] | SpMatElem::ToSparse,
				sparse1 = sparse[1] | SpMatElem::ToSparse;
			incrementKernel(kernel, sparse0, sparse1);

			THEN("Importance of regions presented in the sparse range is incremented by the corresponding values") {
				CHECK_THAT(kernel_span, RangeEquals(sumDouble(sparse[0], sparse[1])));
			}

			AND_WHEN("Decremented with the same sparse region importance ranges") {
				decrementKernel(kernel, sparse0, sparse1);
				then_kernel_is_cleared();
			}

		}

	}

}

SCENARIO("Sparse: Well-defined container property behaviours", "[Container][SplatKernel]") {

	GIVEN("A default initialised sparse splat kernel") {
		Sparse kernel;

		THEN("Kernel is empty") {
			REQUIRE_THAT(kernel, SizeIs(0U));
			REQUIRE_THAT(kernel, IsEmpty());
		}

		WHEN("Kernel is resized") {
			kernel.resize(RegionCount);

			THEN("Kernel remains empty") {
				REQUIRE_THAT(kernel, IsEmpty());
			}

			AND_GIVEN("Some region importances") {
				static constexpr Sparse::IndexType HalfRegionCount = RegionCount / 2U;
				const auto rg = GENERATE(take(3U, chunk(2U, random<Sparse::IndexType>(0U, HalfRegionCount - 1U))));
				const Sparse::IndexType rg0 = rg[0], rg1 = rg[1] + HalfRegionCount;

				WHEN("Kernel is incremented and decremented with these") {

					THEN("Content changes are predictable: incrementation and decrementation inserts or removes values on demand") {
						const auto check_content = [&kernel]<SpMatElem::ImportanceRange Importance>
						requires sized_range<Importance>
						(const Importance importance) -> void {
							CHECK_THAT(kernel.span(), RangeEquals(importance));
						};
						kernel.increment(rg0);
						check_content(to_array<Importance>({
							{ rg0, 1U }
						}));

						kernel.increment(rg0);
						check_content(to_array<Importance>({
							{ rg0, 2U }
						}));

						kernel.increment(rg1);
						check_content(to_array<Importance>({
							{ rg0, 2U },
							{ rg1, 1U }
						}));

						kernel.decrement(rg0);
						check_content(to_array<Importance>({
							{ rg0, 1U },
							{ rg1, 1U }
						}));

						kernel.decrement(rg0);
						check_content(to_array<Importance>({
							{ rg1, 1U }
						}));

						kernel.increment(rg0);
						check_content(to_array<Importance>({
							{ rg1, 1U },
							{ rg0, 1U }
						}));

						AND_WHEN("Kernel is cleared") {
							kernel.clear();

							THEN("Kernel becomes empty") {
								REQUIRE_THAT(kernel, IsEmpty());
							}

						}

					}

				}

			}

		}

	}

}

SCENARIO("Sparse: An array of sparse matrix elements whose values are region importance", "[Container][SplatKernel]") {

	GIVEN("An empty sparse splat kernel with memory allocated") {
		Sparse kernel;
		kernel.resize(RegionCount);

		const auto then_kernel_is_empty = [&kernel] -> void {
			THEN("Kernel is empty") {
				CHECK_THAT(kernel, IsEmpty());
			}
		};

		WHEN("Incremented with region identifiers") {
			const auto rg = generatePositiveRegionIdentifier();
			incrementKernel(kernel, 0U, rg);

			THEN("Element of such regions is inserted and incremented by one per incrementation") {
				CHECK_THAT(kernel.span(), RangeEquals(to_array<Importance>({
					{ 0U, 1U },
					{ rg, 2U }
				})));
			}

			AND_WHEN("Decremented with the same region identifiers") {
				decrementKernel(kernel, 0U, rg);
				then_kernel_is_empty();
			}

		}

		WHEN("Incremented with sparse matrix elements") {
			const auto [elem0, elem1] = generateSparseMatrixElement();
			incrementKernel(kernel, elem0, elem1);

			THEN("Element of such regions is inserted and incremented by their element values per incrementation") {
				const auto [rg1, value1] = elem1;
				CHECK_THAT(kernel.span(), RangeEquals(to_array<Importance>({
					elem0,
					{ rg1, value1 * 2U }
				})));
			}

			AND_WHEN("Decremented with the same sparse matrix elements") {
				decrementKernel(kernel, elem0, elem1);
				then_kernel_is_empty();
			}

		}

		WHEN("Incremented with sparse region importance ranges") {
			const auto sparse = generateSparseImportance();
			auto sparse0 = sparse[0] | SpMatElem::ToSparse,
				sparse1 = sparse[1] | SpMatElem::ToSparse;
			incrementKernel(kernel, sparse0, sparse1);

			THEN("Element of regions presented in the sparse range is inserted and incremented by the corresponding values") {
				SUCCEED("Content checking is too difficult in this case because elements in a sparse kernel are unordered.");
			}

			AND_WHEN("Decremented with the same sparse region importance ranges") {
				decrementKernel(kernel, sparse0, sparse1);
				then_kernel_is_empty();
			}

		}

	}

}