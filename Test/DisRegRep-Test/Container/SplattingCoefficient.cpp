#include <DisRegRep/Container/SplattingCoefficient.hpp>

#include <DisRegRep/Container/SparseMatrixElement.hpp>

#include <DisRegRep/Core/Arithmetic.hpp>
#include <DisRegRep/Core/Type.hpp>

#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>

#include <catch2/matchers/catch_matchers_container_properties.hpp>
#include <catch2/matchers/catch_matchers_quantifiers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <functional>
#include <ranges>

#include <concepts>
#include <type_traits>

#include <cmath>

namespace SpltCoef = DisRegRep::Container::SplattingCoefficient;
namespace SpMatElem = DisRegRep::Container::SparseMatrixElement;
namespace Arithmetic = DisRegRep::Core::Arithmetic;
namespace Type = DisRegRep::Core::Type;

using Catch::Matchers::SizeIs, Catch::Matchers::IsEmpty,
	Catch::Matchers::AllTrue, Catch::Matchers::ContainsSubstring;

using std::ranges::fold_left_first, std::ranges::copy, std::ranges::equal,
	std::bind_back, std::multiplies, std::bit_or,
	std::views::zip_transform, std::views::transform;
using std::unsigned_integral, std::is_unsigned_v;

namespace {

template<unsigned_integral IndexType>
[[nodiscard]] auto generateDimension() {
	return GENERATE(take(2U, chunk(3U, random<IndexType>(3U, 8U))));
}

}

#define MATRIX_TYPE_PRODUCT_LIST (SpltCoef::BasicDense, SpltCoef::BasicSparse), (Type::RegionImportance, Type::RegionMask)

TEMPLATE_PRODUCT_TEST_CASE("Splatting coefficient matrix allocates memory in two dimensions", "[Container][SplattingCoefficient]",
	MATRIX_TYPE_PRODUCT_LIST) {
	using MatrixType = TestType;
	using IndexType = typename MatrixType::IndexType;
	using Dimension3Type = typename MatrixType::Dimension3Type;

	static constexpr bool IsDense = SpltCoef::IsDense<MatrixType>;

	GIVEN("A default constructed region splatting coefficient matrix") {
		MatrixType matrix;

		THEN("Matrix is empty") {
			REQUIRE_THAT(matrix, SizeIs(0U));
			REQUIRE_THAT(matrix, IsEmpty());
			REQUIRE(matrix.sizeByte() == 0U);
		}

		WHEN("Resized") {
			const auto dim = generateDimension<IndexType>();
			REQUIRE_NOTHROW(matrix.resize(Dimension3Type(dim[0], dim[1], dim[2])));

			AND_WHEN("Some sizes are zeros") {

				THEN("Resize fails") {
					if constexpr (IsDense) {
						CHECK_THROWS_WITH(matrix.resize(Dimension3Type(0U, dim[1], dim[2])),
							ContainsSubstring("greaterThan") && ContainsSubstring("0U"));
					}
					CHECK_THROWS_WITH(matrix.resize(Dimension3Type(dim[0], 0U, dim[2])),
						ContainsSubstring("greaterThan") && ContainsSubstring("0U"));
					CHECK_THROWS_WITH(matrix.resize(Dimension3Type(dim[0], dim[1], 0U)),
						ContainsSubstring("greaterThan") && ContainsSubstring("0U"));
				}

			}

			THEN("Matrix memory is allocated with the correct size") {
				if constexpr (IsDense) {
					REQUIRE_THAT(matrix, SizeIs(*fold_left_first(dim, multiplies {})));
					REQUIRE_THAT(matrix, !IsEmpty());
				} else {
					REQUIRE_THAT(matrix, IsEmpty());
				}
			}

		}

	}

}

TEMPLATE_PRODUCT_TEST_CASE("Matrix that stores region splatting coefficients", "[Container][SplattingCoefficient]",
	MATRIX_TYPE_PRODUCT_LIST) {
	using MatrixType = TestType;
	using IndexType = typename MatrixType::IndexType;
	using Dimension3Type = typename MatrixType::Dimension3Type;
	using ValueType = typename MatrixType::ValueType;

	static constexpr bool IsSparse = SpltCoef::IsSparse<MatrixType>;

	GIVEN("A region splatting coefficient matrix and some coefficients") {
		const auto dim = generateDimension<IndexType>();
		MatrixType matrix;
		matrix.resize(Dimension3Type(dim[0], dim[1], dim[2]));

		//I am not going to test sparse input on sparse matrix, since sparse SCM internally views dense input as sparse anyway.
		const auto coefficient = GENERATE_REF(take(1U, chunk(matrix.size(), map(
			[](const auto coef) static noexcept {
				if constexpr (is_unsigned_v<ValueType>) {
					return static_cast<ValueType>(std::round(std::abs(coef)) * 7800U);
				} else {
					return coef;
				}
			}, random(-10.0F, 10.0F)))));

		WHEN("Matrix is filled in with the coefficients") {
			const auto input = coefficient | Arithmetic::View2d(dim[0]);
			const auto output = matrix.range();
			copy(input, output.begin());

			THEN("Coefficients in the matrix equal the input coefficients") {
				auto input_compatible = [&input] constexpr noexcept {
					if constexpr (IsSparse) {
						return input | transform(bind_back(bit_or {}, SpMatElem::ToSparse));
					} else {
						return input;
					}
				}();
				CHECK_THAT(zip_transform(
					equal,
					//ranges::views::indirect would be a perfect fit, just a pity it is not compatible with std::views.
					output | transform([](const auto proxy) static constexpr noexcept { return *proxy; }),
					input_compatible
				), AllTrue());
			}

		}

	}

}