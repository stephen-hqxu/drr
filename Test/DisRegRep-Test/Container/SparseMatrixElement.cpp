#include <DisRegRep/Container/SparseMatrixElement.hpp>

#include <DisRegRep/Core/Type.hpp>

#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <functional>
#include <ranges>

#include <cstdint>

namespace SpMatElem = DisRegRep::Container::SparseMatrixElement;
namespace Type = DisRegRep::Core::Type;

using std::ranges::copy, std::ranges::lower_bound,
	std::mem_fn,
	std::views::transform, std::views::iota, std::views::stride,
	std::views::enumerate, std::views::as_const, std::views::repeat;

SCENARIO("ToDense: Convert from a range of sparse matrix elements to dense values where each element corresponds to its region identifier", "[Container][SparseMatrixElement]") {
	using SparseType = SpMatElem::Importance;

	GIVEN("A range of sparse matrix elements") {
		const auto size = GENERATE(take(3U, random<std::uint_fast8_t>(1U, 10U)));
		auto sparse = GENERATE_COPY(
			take(1U, chunk(size, map([](const auto value) static constexpr noexcept { return SparseType { .Value = value }; },
									 random<SparseType::ValueType>(100U, 100'000U)))));
		const auto sparse_region_id = sparse | transform(mem_fn(&SparseType::Identifier));
		copy(iota(Type::RegionIdentifier {}) | stride(3U) | std::views::take(size), sparse_region_id.begin());

		WHEN("Sparse range is viewed as a dense range") {
			//Use an arbitrary fill value that does not present in the sparse value.
			static constexpr SparseType::ValueType FillValue = 87U;
			auto dense_view = sparse | as_const | SpMatElem::ToDense(size, FillValue);

			THEN("Dense range view is equivalent to the original sparse range") {
				for (const auto [region_id, dense_value] : dense_view | enumerate) [[likely]] {
					if (const auto sparse_it = lower_bound(sparse | as_const, region_id, {}, mem_fn(&SparseType::Identifier));
						sparse_it->Identifier == region_id) {
						CHECK(dense_value == sparse_it->Value);
					} else [[likely]] {
						CHECK(dense_value == FillValue);
					}
				}
			}

		}

	}

}

SCENARIO("ToSparse: Convert from a range of dense values to sparse matrix elements that uses the corresponding index as region identifier", "[Container][SparseMatrixElement]") {
	using SparseType = SpMatElem::Importance;

	GIVEN("A range of dense values") {
		static constexpr SparseType::ValueType IgnoreValue = 17U;

		const auto size = GENERATE(take(3U, random<std::uint_fast8_t>(1U, 50U)));
		auto dense = GENERATE_COPY(take(1U, chunk(size, random<SparseType::ValueType>(25U, 70000U))));
		//Dropout some values.
		const auto dense_dropout = dense | stride(4U);
		copy(repeat(IgnoreValue, dense_dropout.size()), dense_dropout.begin());

		WHEN("Dense range is viewed as a sparse range") {
			auto sparse_view = dense | as_const | SpMatElem::ToSparse(IgnoreValue);

			THEN("Sparse range view is equivalent to the original dense range") {
				for (const auto [region_id, sparse_value] : sparse_view) [[likely]] {
					CHECK(sparse_value != IgnoreValue);
					CHECK(sparse_value == dense[region_id]);
				}
			}

		}

	}

}