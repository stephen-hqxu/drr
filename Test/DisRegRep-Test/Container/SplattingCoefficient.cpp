#include <DisRegRep/Container/SplattingCoefficient.hpp>

#include <DisRegRep/Container/SparseMatrixElement.hpp>

#include <DisRegRep/Core/View/Functional.hpp>
#include <DisRegRep/Core/View/Matrix.hpp>
#include <DisRegRep/Core/Type.hpp>

#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>

#include <catch2/matchers/catch_matchers_container_properties.hpp>
#include <catch2/matchers/catch_matchers_quantifiers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include <glm/gtc/type_ptr.hpp>

#include <span>

#include <algorithm>
#include <functional>
#include <ranges>

#include <concepts>
#include <type_traits>

#include <cmath>

namespace SpltCoef = DisRegRep::Container::SplattingCoefficient;
namespace SpMatElem = DisRegRep::Container::SparseMatrixElement;
namespace View = DisRegRep::Core::View;
namespace Type = DisRegRep::Core::Type;

using Catch::Matchers::SizeIs, Catch::Matchers::IsEmpty,
	Catch::Matchers::AllTrue, Catch::Matchers::ContainsSubstring;

using glm::make_vec3, glm::value_ptr;

using std::span;
using std::ranges::fold_left_first, std::ranges::copy, std::ranges::equal,
	std::bind_front, std::bind_back, std::multiplies, std::bit_or,
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
			const auto dim = make_vec3(generateDimension<IndexType>().data());
			REQUIRE_NOTHROW(matrix.resize(dim));

			AND_WHEN("Some sizes are zeros") {

				THEN("Resize fails") {
					CHECK_THROWS_WITH(matrix.resize(Dimension3Type(0U, dim.y, dim.z)),
						ContainsSubstring("greaterThan") && ContainsSubstring("0U"));
					CHECK_THROWS_WITH(matrix.resize(Dimension3Type(dim.x, 0U, dim.z)),
						ContainsSubstring("greaterThan") && ContainsSubstring("0U"));
					if constexpr (IsDense) {
						CHECK_THROWS_WITH(matrix.resize(Dimension3Type(dim.x, dim.y, 0U)),
							ContainsSubstring("greaterThan") && ContainsSubstring("0U"));
					}
				}

			}

			THEN("Matrix memory is allocated with the correct size") {
				if constexpr (IsDense) {
					REQUIRE_THAT(matrix, SizeIs(*fold_left_first(span(value_ptr(dim), Dimension3Type::length()), multiplies {})));
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
	using ValueType = typename MatrixType::ValueType;

	static constexpr bool IsSparse = SpltCoef::IsSparse<MatrixType>;

	GIVEN("A region splatting coefficient matrix and some coefficients") {
		const auto dim = make_vec3(generateDimension<IndexType>().data());
		MatrixType matrix;
		matrix.resize(dim);

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
			const auto input = coefficient | View::Matrix::View2d(dim.z);
			const auto output = matrix.range();
			copy(input, output.begin());

			THEN("Coefficients in the matrix equal the input coefficients") {
				const auto equal_2d = bind_front(zip_transform, equal, output | View::Functional::Dereference);
				if constexpr (IsSparse) {
					CHECK_THAT(equal_2d(input | transform(bind_back(bit_or {}, SpMatElem::ToSparse))), AllTrue());
				} else {
					CHECK_THAT(equal_2d(input), AllTrue());
				}
			}

		}

	}

}