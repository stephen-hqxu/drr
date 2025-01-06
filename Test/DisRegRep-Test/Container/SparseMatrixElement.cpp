#include <DisRegRep/Container/SparseMatrixElement.hpp>

#include <DisRegRep/Core/Type.hpp>

#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>

#include <catch2/matchers/catch_matchers_container_properties.hpp>
#include <catch2/matchers/catch_matchers_predicate.hpp>
#include <catch2/matchers/catch_matchers_quantifiers.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include <range/v3/range/conversion.hpp>
#include <range/v3/view/const.hpp>

#include <vector>

#include <tuple>

#include <algorithm>
#include <functional>
#include <iterator>
#include <ranges>

#include <type_traits>

#include <cstdint>

namespace SpMatElem = DisRegRep::Container::SparseMatrixElement;
namespace Type = DisRegRep::Core::Type;

using Catch::Matchers::RangeEquals, Catch::Matchers::AllMatch, Catch::Matchers::Predicate, Catch::Matchers::SizeIs;

using ranges::views::const_;

using std::vector,
	std::tuple, std::make_from_tuple;
using std::ranges::copy, std::ranges::fill,
	std::mem_fn, std::bind_front, std::bind_back, std::bit_or, std::ranges::equal_to,
	std::ranges::distance,
	std::views::transform, std::views::iota, std::views::enumerate,
	std::views::stride, std::views::join, std::views::drop,
	std::views::as_const;
using std::is_arithmetic_v, std::is_floating_point_v;

namespace {

using SparseTypeList = std::tuple<SpMatElem::Importance, SpMatElem::Mask>;

template<typename ValueType>
requires is_arithmetic_v<ValueType>
[[nodiscard]] auto makeRandomGenerator() {
	using Catch::Generators::random;
	if constexpr (is_floating_point_v<ValueType>) {
		return random<ValueType>(-100.0F, 100.0F);
	} else {
		return random<ValueType>(100U, 100'000U);
	}
}

}

TEMPLATE_LIST_TEST_CASE("ToDense: Convert from a range of sparse matrix elements to dense values where each element corresponds to its region identifier", "[Container][SparseMatrixElement]", SparseTypeList) {
	using SparseType = TestType;
	using ValueType = typename SparseType::ValueType;

	GIVEN("A range of sparse matrix elements") {
		static constexpr Type::RegionIdentifier RegionIdentifierStride = 4U;

		const auto size = GENERATE(take(3U, random<std::uint_fast8_t>(1U, 10U)));
		auto sparse = GENERATE_COPY(
			take(1U, chunk(size, map([](const auto value) static constexpr noexcept { return SparseType { .Value = value }; },
									 makeRandomGenerator<ValueType>()))));

		using std::views::take;
		const auto sparse_region_id = sparse | transform(mem_fn(&SparseType::Identifier));
		copy(iota(Type::RegionIdentifier {}) | stride(RegionIdentifierStride) | take(size), sparse_region_id.begin());

		WHEN("Sparse range is viewed as a dense range") {
			using ranges::to;
			//Use an arbitrary fill value that does not present in the sparse value.
			static constexpr ValueType FillValue = 87U;
			static constexpr std::uint_fast8_t DenseSize = 25U;

			//Make it into a persistent storage since it is only an input range.
			const auto dense_view = sparse | const_ | SpMatElem::ToDense(DenseSize, FillValue) | to<vector>;

			THEN("Dense range view is equivalent to the original sparse range") {
				using std::views::chunk;
				const std::uint_fast16_t stride_size = size * RegionIdentifierStride;

				const auto dense_view_no_padding = dense_view | take(stride_size);
				const auto dense_value = dense_view_no_padding | stride(RegionIdentifierStride);
				auto dense_fill =
					dense_view_no_padding | chunk(RegionIdentifierStride) | transform(bind_back(bit_or {}, drop(1U))) | join;

				const auto match_fill_value = AllMatch(Predicate<ValueType>(bind_front(equal_to {}, FillValue),
					"Region identifier that does not present in the sparse range should be assigned with a specified fixed value."));
				CHECK_THAT(dense_value, RangeEquals(sparse | transform(mem_fn(&SparseType::Value))));
				CHECK_THAT(dense_fill, match_fill_value);

				CHECK_THAT(dense_view, SizeIs(DenseSize));
				CHECK_THAT(dense_view | drop(stride_size), match_fill_value);
			}

		}

	}

}

TEMPLATE_LIST_TEST_CASE("ToSparse: Convert from a range of dense values to sparse matrix elements that uses the corresponding index as region identifier", "[Container][SparseMatrixElement]", SparseTypeList) {
	using SparseType = TestType;
	using ValueType = typename SparseType::ValueType;

	GIVEN("A range of dense values") {
		static constexpr Type::RegionIdentifier RegionIdentifierStride = 6U;
		static constexpr ValueType IgnoreValue = 17U;

		const auto size = GENERATE(take(3U, random<std::uint_fast8_t>(1U, 50U)));
		auto dense = GENERATE_COPY(take(1U, chunk(size, makeRandomGenerator<ValueType>())));

		fill(dense | stride(RegionIdentifierStride), IgnoreValue);

		WHEN("Dense range is viewed as a sparse range") {
			auto sparse_view = dense | as_const | SpMatElem::ToSparse(IgnoreValue);

			THEN("Sparse range view is equivalent to the original dense range") {
				using std::views::chunk;
				auto dense_value = dense
					| enumerate
					| chunk(RegionIdentifierStride)
					| transform(bind_back(bit_or {}, drop(1U)))
					| join
					| transform([](const auto it) static constexpr noexcept { return make_from_tuple<SparseType>(it); });

				CHECK_THAT(sparse_view, RangeEquals(dense_value));
				CHECK(distance(sparse_view) == distance(dense_value));
			}

		}

	}

}