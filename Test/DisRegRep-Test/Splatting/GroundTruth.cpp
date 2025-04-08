#include <DisRegRep-Test/Splatting/GroundTruth.hpp>

#include <DisRegRep/Container/Regionfield.hpp>
#include <DisRegRep/Container/SparseMatrixElement.hpp>
#include <DisRegRep/Container/SplattingCoefficient.hpp>

#include <DisRegRep/Core/View/Arithmetic.hpp>
#include <DisRegRep/Core/View/Functional.hpp>
#include <DisRegRep/Core/MdSpan.hpp>
#include <DisRegRep/Core/Type.hpp>

#include <DisRegRep/Splatting/Convolution/Full/Base.hpp>
#include <DisRegRep/Splatting/Container.hpp>

#include <DisRegRep-Test/StringMaker.hpp>

#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/catch_test_macros.hpp>

#include <glm/gtc/type_ptr.hpp>

#include <array>

#include <any>
#include <tuple>

#include <algorithm>
#include <functional>
#include <iterator>
#include <ranges>

#include <utility>

#include <concepts>

#include <cstdint>

namespace GndTth = DisRegRep::Test::Splatting::GroundTruth;
namespace SpMatElem = DisRegRep::Container::SparseMatrixElement;
namespace SpltCoef = DisRegRep::Container::SplattingCoefficient;
namespace View = DisRegRep::Core::View;
namespace Type = DisRegRep::Core::Type;
namespace Splt = DisRegRep::Splatting;
using DisRegRep::Container::Regionfield,
	DisRegRep::Core::MdSpan::reverse;

using Catch::Matchers::WithinAbs, Catch::Matchers::RangeEquals, Catch::Matchers::ContainsSubstring;

using glm::make_vec2;

using std::array, std::to_array;
using std::any,
	std::tie, std::apply, std::tuple_size_v;
using std::ranges::copy, std::ranges::all_of,
	std::bind_front, std::bind_back, std::identity,
	std::indirect_binary_predicate,
	std::views::transform, std::views::join, std::views::zip_transform,
	std::ranges::input_range, std::ranges::viewable_range,
	std::ranges::range_value_t, std::ranges::range_const_reference_t, std::ranges::const_iterator_t;
using std::floating_point;

namespace {

namespace Reference {

namespace Regionfield {

constexpr auto Dimension = ::Regionfield::DimensionType(6U, 8U);
constexpr auto Value = to_array<::Regionfield::ValueType>({
	0, 2, 1, 2, 1, 2, 3, 2,
	0, 3, 0, 2, 2, 3, 3, 0,
	0, 3, 2, 3, 0, 3, 0, 1,
	0, 3, 1, 0, 3, 2, 0, 2,
	0, 2, 3, 3, 1, 1, 1, 3,
	0, 2, 3, 1, 1, 2, 3, 2
});
constexpr auto RegionCount = std::ranges::max(Value) + 1U;

[[nodiscard]] ::Regionfield load(const bool transpose) {
	::Regionfield rf;
	rf.RegionCount = RegionCount;
	rf.resize(Dimension);
	copy(Value, rf.span().begin());
	return transpose ? rf.transpose() : std::move(rf);
}

}

namespace Convolution::Full {

using Splt::Convolution::Full::Base;

constexpr Base::KernelSizeType Radius = 2U;
constexpr auto Offset = Base::DimensionType(Radius, Radius + 1U),
	OffsetTransposed = reverse(Offset),
	Extent = Base::DimensionType(2U, 3U),
	ExtentTransposed = reverse(Extent);

template<typename T>
using SplattingCoefficientMatrixType = array<array<T, Regionfield::RegionCount>, Extent.x * Extent.y>;

constexpr auto SplattingCoefficientMatrixDense = [] static consteval noexcept {
	static constexpr SplattingCoefficientMatrixType<Type::RegionImportance> Importance {{
		{ 3, 5, 8, 9 }, { 5, 6, 6, 8 }, { 5, 5, 7, 8 },
		{ 3, 5, 7, 10 }, { 5, 6, 5, 9 }, { 5, 6, 6, 8 }
	}};
	static constexpr Type::RegionMask NormFactor = Base::kernelNormalisationFactor(Base::diametre(Radius));

	SplattingCoefficientMatrixType<Type::RegionMask> mask {};
	auto mask_join = mask | join;
	copy(Importance | transform(bind_back(View::Arithmetic::Normalise, NormFactor)) | join, mask_join.begin());
	return mask;
}();
constexpr auto SplattingCoefficientMatrixSparse = [] static consteval noexcept {
	SplattingCoefficientMatrixType<SpMatElem::Mask> sparse_mask {};
	auto sparse_mask_join = sparse_mask | join;
	copy(SplattingCoefficientMatrixDense | transform(bind_front(SpMatElem::ToSparse)) | join, sparse_mask_join.begin());
	return sparse_mask;
}();

bool compare(const floating_point auto source, const floating_point auto target) {
	return WithinAbs(target, 1e-6F).match(source);
}

template<
	SpltCoef::Is Matrix,
	input_range Ref,
	indirect_binary_predicate<
		typename range_value_t<decltype(std::declval<const Matrix&>().range())>::ProxyElementViewIterator,
		const_iterator_t<range_const_reference_t<Ref>>
	> Comp
> requires viewable_range<range_const_reference_t<Ref>>
void compare(const Matrix& matrix, const Ref& reference, Comp comp) {
	CHECK(typename Matrix::Dimension2Type(matrix.extent()) == Extent);
	CHECK_THAT(matrix.range() | View::Functional::Dereference, RangeEquals(reference, [&comp](const auto source, const auto target) {
		return all_of(zip_transform(comp, source, target), identity {});
	}));
}

template<SpltCoef::IsDense Matrix>
void compare(const Matrix& matrix) {
	compare(matrix, SplattingCoefficientMatrixDense, compare<Type::RegionMask, Type::RegionMask>);
}

template<SpltCoef::IsSparse Matrix>
void compare(Matrix& matrix) {
	matrix.sort();
	compare(matrix, SplattingCoefficientMatrixSparse, [](const auto source, const auto target) static {
		const auto [src_region_id, src_value] = source;
		const auto [tgt_region_id, tgt_value] = target;
		return src_region_id == tgt_region_id && compare(src_value, tgt_value);
	});
}

}

}

}

void GndTth::checkMinimumRequirement(BaseConvolution& splatting) {
	THEN("It has the correct minimum requirements") {
		const auto size = GENERATE(take(3U, chunk(5U, random<std::uint_least8_t>(16U, 128U))));
		const BaseConvolution::InvokeInfo invoke_info {
			.Offset = make_vec2(size.data()),
			.Extent = make_vec2(size.data() + 2U)
		};
		const auto& [offset, extent] = invoke_info;
		splatting.Radius = size.back();

		REQUIRE(splatting.minimumRegionfieldDimension(invoke_info) == extent + offset + splatting.Radius);
		REQUIRE(splatting.minimumOffset() == BaseConvolution::DimensionType(splatting.Radius));
	}
}

//NOLINTNEXTLINE(readability-function-cognitive-complexity)
void GndTth::checkSplattingCoefficient(BaseFullConvolution& splatting) {
	AND_GIVEN("An invoke specification that does not satisfy the minimum requirements") {
		const auto size = GENERATE(take(3U, chunk(4U, random<BaseFullConvolution::KernelSizeType>(4U, 8U))));
		const auto extent = make_vec2(size.data());
		splatting.Radius = size[2U];

		BaseFullConvolution::InvokeInfo optimal_invoke_info {
			.Offset = splatting.minimumOffset(),
			.Extent = extent
		};

		Regionfield rf;
		rf.RegionCount = size.back();
		rf.resize(splatting.minimumRegionfieldDimension(optimal_invoke_info));

		auto invoke = [
			&splatting = std::as_const(splatting),
			&invoke_info = std::as_const(optimal_invoke_info),
			&rf = std::as_const(rf),
			memory = any()
		]() mutable -> void {
			apply(
				[&](const auto... trait) { (splatting(trait, rf, memory, invoke_info), ...); }, Splt::Container::Combination);
		};

		WHEN("Regionfield is too small") {
			rf.resize(rf.extent() - 1U);

			REQUIRE_THROWS_WITH(invoke(), ContainsSubstring("greaterThanEqual") && ContainsSubstring("minimumRegionfieldDimension"));
		}

		WHEN("Offset is too small") {
			optimal_invoke_info.Offset -= 1U;

			REQUIRE_THROWS_WITH(invoke(), ContainsSubstring("greaterThanEqual") && ContainsSubstring("minimumOffset"));
		}

	}

	WHEN("It is invoked with ground truth data") {
		namespace CurrentRef = Reference::Convolution::Full;
		array<any, tuple_size_v<Splt::Container::CombinationType>> memory;

		splatting.Radius = CurrentRef::Radius;
		const auto result = apply([&splatting = std::as_const(splatting), &memory](const auto... trait) {
			return apply([&splatting, trait...](auto&... memory) {
				const bool transposed = splatting.isTransposed();
				const Regionfield rf = Reference::Regionfield::load(transposed);

				const BaseFullConvolution::InvokeInfo invoke_info {
					.Offset = transposed ? CurrentRef::OffsetTransposed : CurrentRef::Offset,
					.Extent = transposed ? CurrentRef::ExtentTransposed : CurrentRef::Extent
				};
				return tie(splatting(trait, rf, memory, invoke_info)...);
			}, memory);
		}, Splt::Container::Combination);

		THEN("Splatting coefficients computed are correct") {
			apply([](auto&... matrix) static { (CurrentRef::compare(matrix), ...); }, result);
		}

	}
}