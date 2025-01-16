#include <DisRegRep-Test/Splatting/GroundTruth.hpp>

#include <DisRegRep/Container/Regionfield.hpp>
#include <DisRegRep/Container/SparseMatrixElement.hpp>
#include <DisRegRep/Container/SplattingCoefficient.hpp>

#include <DisRegRep/Core/Arithmetic.hpp>
#include <DisRegRep/Core/Type.hpp>

#include <DisRegRep/Splatting/Convolution/Full/Base.hpp>
#include <DisRegRep/Splatting/Trait.hpp>

#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>

#include <any>
#include <tuple>

#include <algorithm>
#include <functional>
#include <iterator>
#include <ranges>

#include <utility>

#include <concepts>

namespace GndTth = DisRegRep::Test::Splatting::GroundTruth;
namespace SpMatElem = DisRegRep::Container::SparseMatrixElement;
namespace SpltCoef = DisRegRep::Container::SplattingCoefficient;
namespace Type = DisRegRep::Core::Type;
namespace Splt = DisRegRep::Splatting;
using DisRegRep::Container::Regionfield,
	DisRegRep::Core::Arithmetic::Normalise;

using Catch::Matchers::WithinAbs, Catch::Matchers::RangeEquals;

using std::array, std::to_array;
using std::any, std::tuple, std::apply;
using std::ranges::copy, std::ranges::all_of,
	std::bind_front, std::bind_back, std::identity,
	std::indirect_binary_predicate,
	std::views::transform, std::views::join, std::views::zip_transform, std::views::all,
	std::ranges::viewable_range,
	std::ranges::range_const_reference_t, std::ranges::const_iterator_t;
using std::floating_point;

namespace {

namespace Reference {

namespace Regionfield {

constexpr auto Dimension = ::Regionfield::DimensionType(6U, 8U);
constexpr auto Value = to_array<::Regionfield::ValueType>({
	0, 0, 0, 0, 0, 0,
	2, 3, 3, 3, 2, 2,
	1, 0, 2, 1, 3, 3,
	2, 2, 3, 0, 3, 1,
	1, 2, 0, 3, 1, 1,
	2, 3, 3, 2, 1, 2,
	3, 3, 0, 0, 1, 3,
	2, 0, 1, 2, 3, 2
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

constexpr Base::RadiusType Radius = 2U;
constexpr auto Offset = Base::DimensionType(Radius, Radius + 1U),
	OffsetTransposed = Base::DimensionType(Offset.y, Offset.x),
	Extent = Base::DimensionType(2U, 3U),
	ExtentTransposed = Base::DimensionType(Extent.y, Extent.x);

template<typename T>
using SplattingCoefficientMatrixType = array<array<T, Regionfield::RegionCount>, Extent.x * Extent.y>;

constexpr auto SplattingCoefficientMatrixDense = [] static consteval noexcept {
	static constexpr SplattingCoefficientMatrixType<Type::RegionImportance> Importance {{
		{ 3, 5, 8, 9 }, { 3, 5, 7, 10 },
		{ 5, 6, 6, 8 }, { 5, 6, 5, 9 },
		{ 5, 5, 7, 8 }, { 5, 6, 6, 8 }
	}};
	static constexpr Type::RegionMask NormFactor = Base::kernelNormalisationFactor(Base::diametre(Radius));

	SplattingCoefficientMatrixType<Type::RegionMask> mask {};
	auto mask_join = mask | join;
	copy(Importance | transform(bind_back(Normalise, NormFactor)) | join, mask_join.begin());
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
	viewable_range Ref,
	indirect_binary_predicate<
		const_iterator_t<typename Matrix::template ValueProxy<true>::ProxyElementViewType>,
		const_iterator_t<range_const_reference_t<Ref>>
	> Comp
>
void compare(const Matrix& matrix, const Ref& reference, Comp comp) {
	CHECK_THAT(matrix.range() | transform([](const auto proxy) static constexpr noexcept { return *proxy; }),
		RangeEquals(reference | all,
			[&comp](const auto source, const auto target) { return all_of(zip_transform(comp, source, target), identity {}); }));
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

void GndTth::checkFullConvolution(Splt::Convolution::Full::Base& full_conv) {
	namespace CurrentRef = Reference::Convolution::Full;

	full_conv.Radius = CurrentRef::Radius;
	apply([&full_conv = std::as_const(full_conv)](const auto... trait) {
		const bool transposed = full_conv.isTransposed();
		const Regionfield rf = Reference::Regionfield::load(transposed);

		const CurrentRef::Base::InvokeInfo invoke_info {
			.Offset = transposed ? CurrentRef::OffsetTransposed : CurrentRef::Offset,
			.Extent = transposed ? CurrentRef::ExtentTransposed : CurrentRef::Extent
		};
		any memory;

		(CurrentRef::compare(full_conv(trait, rf, memory, invoke_info)), ...);
	}, tuple<
		DRR_SPLATTING_TRAIT_CONTAINER(Dense, Dense),
		DRR_SPLATTING_TRAIT_CONTAINER(Dense, Sparse),
		DRR_SPLATTING_TRAIT_CONTAINER(Sparse, Sparse)
	> {});
}