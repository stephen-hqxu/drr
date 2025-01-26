#include <DisRegRep/Container/SplatKernel.hpp>

#include <DisRegRep/Container/SparseMatrixElement.hpp>

#include <DisRegRep/Core/Type.hpp>

#include <catch2/generators/catch_generators.hpp>

#include <catch2/matchers/catch_matchers_container_properties.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <span>
#include <tuple>

#include <algorithm>
#include <functional>
#include <ranges>

#include <utility>

#include <type_traits>

#include <cstdint>

namespace SpltKn = DisRegRep::Container::SplatKernel;
namespace SpMatElem = DisRegRep::Container::SparseMatrixElement;
namespace Type = DisRegRep::Core::Type;
using SpltKn::Dense, SpltKn::Sparse;

using Catch::Matchers::SizeIs, Catch::Matchers::IsEmpty,
	Catch::Matchers::RangeEquals;

using std::to_array, std::span,
	std::apply;
using std::ranges::all_of, std::ranges::copy, std::ranges::for_each,
	std::bind_back, std::ranges::greater_equal, std::identity,
	std::views::zip_transform, std::views::repeat, std::views::enumerate;
using std::remove_const_t, std::is_same_v;

namespace {

constexpr auto DenseIncrement0 = to_array<Type::RegionImportance>({ 99, 33, 0, 86, 0, 66, 12, 91, 60, 85 }),
			   DenseDecrement0 = to_array<Type::RegionImportance>({ 57, 0, 0, 86, 0, 66, 0, 28, 0, 85 }),
			   DenseIncrement1 = to_array<Type::RegionImportance>({ 0, 98, 0, 48, 10, 0, 70, 0, 74, 50 });
static_assert(all_of(zip_transform(greater_equal {}, DenseIncrement0, DenseDecrement0), identity {}),
	"Having decrement amount more than that of the increment will cause integer underflow.");

constexpr auto RegionCount = DenseIncrement0.size();

constexpr auto ExpectedDense = [] static consteval noexcept {
	remove_const_t<decltype(DenseIncrement0)> result {};
	copy(zip_transform([](const auto a, const auto b, const auto c) static consteval noexcept { return a - b + c; },
		DenseIncrement0, DenseDecrement0, DenseIncrement1),
		result.begin());
	return result;
}();
//NOLINTBEGIN(modernize-use-designated-initializers)
constexpr auto ExpectedSparse = to_array<SpMatElem::Importance>({
	{ 0, 42 },
	{ 1, 131 },
	{ 6, 82 },
	{ 7, 63 },
	{ 8, 134 },
	{ 3, 48 },
	{ 4, 10 },
	{ 9, 50 }
});
//NOLINTEND(modernize-use-designated-initializers)

enum class ModifyType : std::uint_fast8_t {
	Dense,
	Sparse,
	DenseArray,
	SparseArray
};

template<SpltKn::Is Kernel>
void modifyKernel(Kernel& kernel, const ModifyType modify_type) {
	const auto increment = [&kernel](const auto inc) -> void {
		kernel.increment(inc);
	};
	const auto decrement = [&kernel](const auto dec) -> void {
		kernel.decrement(dec);
	};

	const auto modify_element_wise = [](const auto tup, const auto& modifier) static -> void {
		for_each(apply(repeat, tup), modifier);
	};
	const auto increment_element_wise = bind_back(modify_element_wise, std::cref(increment));
	const auto decrement_element_wise = bind_back(modify_element_wise, std::cref(decrement));

	using enum ModifyType;
	switch (modify_type) {
	case Dense:
		for_each(DenseIncrement0 | enumerate, increment_element_wise);
		for_each(DenseDecrement0 | enumerate, decrement_element_wise);
		for_each(DenseIncrement1 | enumerate, increment_element_wise);
		break;
	case Sparse:
		for_each(DenseIncrement0 | SpMatElem::ToSparse, increment);
		for_each(DenseDecrement0 | SpMatElem::ToSparse, decrement);
		for_each(DenseIncrement1 | SpMatElem::ToSparse, increment);
		break;
	case DenseArray:
		if constexpr (is_same_v<Kernel, SpltKn::Dense>) {
			kernel.increment(DenseIncrement0);
			kernel.decrement(DenseDecrement0);
			kernel.increment(DenseIncrement1);
			break;
		} else {
			std::unreachable();
		}
	case SparseArray:
		kernel.increment(DenseIncrement0 | SpMatElem::ToSparse);
		kernel.decrement(DenseDecrement0 | SpMatElem::ToSparse);
		kernel.increment(DenseIncrement1 | SpMatElem::ToSparse);
		break;
	default: std::unreachable();
	}
}

}

TEMPLATE_TEST_CASE("A dense array of region importance where index corresponds to region identifier, or a sparse array of sparse matrix elements whose values are region importance", "[Container][SplatKernel]", Dense, Sparse) {
	using KernelType = TestType;
	static constexpr bool IsDense = is_same_v<KernelType, Dense>;

	GIVEN("A default initialised splat kernel") {
		KernelType kernel;

		THEN("Kernel is empty") {
			REQUIRE_THAT(kernel, SizeIs(0U));
			REQUIRE_THAT(kernel, IsEmpty());
		}

		WHEN("Kernel is resized") {
			kernel.resize(RegionCount);

			if constexpr (IsDense) {
				THEN("Kernel has the correct size") {
					REQUIRE_THAT(kernel, SizeIs(RegionCount));
				}

				THEN("Region importances are all zeros") {
					REQUIRE_THAT(kernel.span(), RangeEquals(repeat(Dense::ValueType {}, RegionCount)));
				}
			} else {
				THEN("Kernel remains empty") {
					REQUIRE_THAT(kernel, IsEmpty());
				}
			}

		}

	}

	GIVEN("An empty splat kernel with memory allocated") {
		KernelType kernel;
		kernel.resize(RegionCount);

		WHEN("Kernel is incremented and decremented") {
			{
				using enum ModifyType;
				if constexpr (IsDense) {
					modifyKernel(kernel, GENERATE(values({ Dense, Sparse, DenseArray, SparseArray })));
				} else {
					modifyKernel(kernel, GENERATE(values({ Dense, Sparse, SparseArray })));
				}
			}

			THEN("Importance of regions are correct") {
				if constexpr (IsDense) {
					CHECK_THAT(kernel.span(), RangeEquals(ExpectedDense));
				} else {
					CHECK_THAT(kernel.span(), RangeEquals(ExpectedSparse));
				}
			}

			AND_WHEN("Kernel is cleared") {
				//Do not take a span from a sparse kernel because memory is dynamically allocated.
				const span old_kernel_memory = kernel.span();
				kernel.clear();

				if constexpr (IsDense) {
					THEN("Kernel size remains unchanged") {
						CHECK_THAT(kernel, SizeIs(RegionCount));

						AND_THEN("Iterators remain valid") {
							REQUIRE(kernel.span().data() == old_kernel_memory.data());
						}
					}

					THEN("Region importances are reset to zeros") {
						CHECK_THAT(old_kernel_memory, RangeEquals(repeat(Dense::ValueType {}, RegionCount)));
					}
				} else {
					THEN("Kernel becomes empty") {
						REQUIRE_THAT(kernel, IsEmpty());
					}
				}

			}

		}

	}

}