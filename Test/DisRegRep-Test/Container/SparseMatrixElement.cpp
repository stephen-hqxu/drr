#include <DisRegRep/Container/SparseMatrixElement.hpp>

#include <DisRegRep/Core/Type.hpp>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <tuple>

#include <algorithm>
#include <ranges>

#include <type_traits>

namespace SpMatElem = DisRegRep::Container::SparseMatrixElement;
namespace Type = DisRegRep::Core::Type;

using std::to_array, std::tuple;
using std::ranges::equal,
	std::views::all;
using std::is_same_v;

namespace {

template<typename Element>
concept ValidElementType = is_same_v<Element, SpMatElem::Importance> || is_same_v<Element, SpMatElem::Mask>;

using ElementTypeList = std::tuple<SpMatElem::Importance, SpMatElem::Mask>;

constexpr auto IndicatorImportance = Type::RegionImportance { 91 };
constexpr auto IndicatorMask = Type::RegionMask { 0.93 };

constexpr auto DenseImportance = to_array<Type::RegionImportance>({
	IndicatorImportance,
	1,
	27,
	IndicatorImportance,
	33,
	24,
	43,
	2,
	IndicatorImportance,
	IndicatorImportance
});
constexpr auto DenseMask = to_array<Type::RegionMask>({ 
	0.61,
	0.45,
	IndicatorMask,
	0.16,
	0.82,
	IndicatorMask,
	IndicatorMask,
	0.08,
	IndicatorMask,
	0.80
});
//NOLINTBEGIN(modernize-use-designated-initializers)
constexpr auto SparseImportance = to_array<SpMatElem::Importance>({
	{ 1, 1 },
	{ 2, 27 },
	{ 4, 33 },
	{ 5, 24 },
	{ 6, 43 },
	{ 7, 2 }
});
constexpr auto SparseMask = to_array<SpMatElem::Mask>({
	{ 0, 0.61 },
	{ 1, 0.45 },
	{ 3, 0.16 },
	{ 4, 0.82 },
	{ 7, 0.08 },
	{ 9, 0.80 }
});
//NOLINTEND(modernize-use-designated-initializers)

template<ValidElementType Element>
[[nodiscard]] consteval auto loadDense() noexcept {
	if constexpr (is_same_v<Element, SpMatElem::Importance>) {
		return tuple(DenseImportance | all, IndicatorImportance);
	} else {
		return tuple(DenseMask | all, IndicatorMask);
	}
}

template<ValidElementType Element>
[[nodiscard]] consteval auto loadSparse() noexcept {
	if constexpr (is_same_v<Element, SpMatElem::Importance>) {
		return tuple(SparseImportance | all, IndicatorImportance);
	} else {
		return tuple(SparseMask | all, IndicatorMask);
	}
}

template<ValidElementType Element>
[[nodiscard]] consteval bool equalDenseView() noexcept {
	const auto [sparse, fill_value] = loadSparse<Element>();
	const auto [dense, _] = loadDense<Element>();

	return equal(sparse | SpMatElem::ToDense(dense.size(), fill_value), dense);
}

template<ValidElementType Element>
[[nodiscard]] consteval bool equalSparseView() noexcept {
	const auto [dense, ignore_value] = loadDense<Element>();
	const auto [sparse, _] = loadSparse<Element>();

	return equal(dense | SpMatElem::ToSparse(ignore_value), sparse);
}

}

TEMPLATE_LIST_TEST_CASE("ToDense: Convert from a range of sparse matrix elements to dense values where each element corresponds to its region identifier", "[Container][SparseMatrixElement]", ElementTypeList) {
	using ElementType = TestType;

	GIVEN("A range of sparse matrix elements") {

		WHEN("Sparse range is viewed as a dense range") {

			THEN("Dense range view is equivalent to the original sparse range") {
				STATIC_CHECK(equalDenseView<ElementType>());
			}

		}

	}

}

TEMPLATE_LIST_TEST_CASE("ToSparse: Convert from a range of dense values to sparse matrix elements that uses the corresponding index as region identifier", "[Container][SparseMatrixElement]", ElementTypeList) {
	using ElementType = TestType;

	GIVEN("A range of dense values") {

		WHEN("Dense range is viewed as a sparse range") {

			THEN("Sparse range view is equivalent to the original dense range") {
				STATIC_CHECK(equalSparseView<ElementType>());
			}

		}

	}

}