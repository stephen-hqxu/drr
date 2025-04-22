#include <DisRegRep/RegionfieldGenerator/DiamondSquare.hpp>
#include <DisRegRep/RegionfieldGenerator/ExecutionPolicy.hpp>
#include <DisRegRep/RegionfieldGenerator/ImplementationHelper.hpp>
#include <DisRegRep/RegionfieldGenerator/Uniform.hpp>

#include <DisRegRep/Container/Regionfield.hpp>

#include <DisRegRep/Core/View/Concat.hpp>
#include <DisRegRep/Core/View/Functional.hpp>
#include <DisRegRep/Core/View/Matrix.hpp>
#include <DisRegRep/Core/Bit.hpp>
#include <DisRegRep/Core/Exception.hpp>
#include <DisRegRep/Core/XXHash.hpp>

#include <glm/fwd.hpp>
#include <glm/common.hpp>
#include <glm/vector_relational.hpp>
#include <glm/vec2.hpp>

#include <array>
#include <bitset>
#include <tuple>

#include <algorithm>
#include <execution>
#include <functional>
#include <ranges>

#include <utility>

#include <concepts>
#include <type_traits>

#include <cassert>
#include <cstdint>

using DisRegRep::RegionfieldGenerator::DiamondSquare,
	DisRegRep::Container::Regionfield;
using DisRegRep::Core::Bit::BitPerSampleResult,
	DisRegRep::Core::XXHash::HashType, DisRegRep::Core::XXHash::SecretView, DisRegRep::Core::XXHash::makeSecret;

using glm::greaterThanEqual;

using std::array, std::bitset,
	std::tuple, std::tie, std::apply;
using std::ranges::fold_left,
	std::execution::unseq, std::is_execution_policy_v,
	std::bind_front, std::bind_back, std::plus,
	std::views::repeat, std::views::iota, std::views::cartesian_product,
	std::views::stride, std::views::transform, std::views::chunk, std::views::zip,
	std::ranges::input_range, std::ranges::forward_range,
	std::ranges::common_range, std::ranges::viewable_range, std::ranges::view,
	std::ranges::range_value_t;
using std::integer_sequence, std::make_integer_sequence;
using std::convertible_to,
	std::invoke_result_t, std::is_same_v, std::conjunction_v,
	std::is_copy_constructible, std::is_nothrow_copy_constructible, std::common_type_t;

namespace {

using LengthType = DiamondSquare::DimensionType::length_type;
using ScalarType = DiamondSquare::DimensionType::value_type;
using GridStrideType = std::uint_fast8_t;

template<const BitPerSampleResult& BpsResult>
using BpsResultConstant = std::integral_constant<const BitPerSampleResult&, BpsResult>;

template<typename R>
concept OffsetRange = input_range<R> && viewable_range<R> && is_same_v<range_value_t<R>, DiamondSquare::DimensionType>;
template<typename F>
concept OffsetRangeMaker = OffsetRange<invoke_result_t<F, DiamondSquare::DimensionType, GridStrideType>>;

template<typename R>
concept GridInputOutputRange = forward_range<R> && common_range<R>;

constexpr auto FirstPassBps = BitPerSampleResult(BitPerSampleResult::DataTypeTag<HashType>, 4U),
			   SecondPassBps = BitPerSampleResult(BitPerSampleResult::DataTypeTag<HashType>, 2U),
			   SmoothPassBps = BitPerSampleResult(BitPerSampleResult::DataTypeTag<HashType>, 1U);
constexpr std::uint_fast8_t SaltSize = 32U;
constexpr auto FirstPassSalt = makeSecret<SaltSize>("fe ab 32 d2 af 0d c2 e9 9c 1f 67 be 74 6c 97 58 05 97 58 f2 29 99 ef 10 34 58 8b bc 81 cc 80 e1"),
			   SecondPassSalt = makeSecret<SaltSize>("29 5c e5 97 b8 07 99 82 f8 5c 14 a5 1d 1b f4 67 04 2a 65 17 f1 2a b2 f3 16 b1 56 ea d5 d2 71 53"),
			   SmoothPassSalt = makeSecret<SaltSize>("26 ce a9 63 d3 74 48 b8 30 65 58 a8 76 b5 6f 9a 9e 71 78 b2 43 2f 0f 32 bc 44 4e c2 3c d9 7a 9b");

[[nodiscard]] constexpr DiamondSquare::DimensionType upscale(const DiamondSquare::DimensionType dim) noexcept {
	return dim * 2U - 1U;
}

constexpr auto MakeOffsetRange = [](
	const DiamondSquare::DimensionType extent,
	const GridStrideType skip,
	const DiamondSquare::DimensionType initial = DiamondSquare::DimensionType(0U),
	const DiamondSquare::DimensionType extent_deduction = DiamondSquare::DimensionType(0U)
) static constexpr noexcept -> view auto {
	using DimensionType = DiamondSquare::DimensionType;
	return [initial, extent = extent - extent_deduction, grid_skip = stride(skip)]<LengthType... I>(
			   integer_sequence<LengthType, I...>) constexpr noexcept {
		//Equivalent to creating an unbounded iota then piping into a take, but we want to keep a sized and common range.
		return cartesian_product(iota(initial[I], initial[I] + extent[I]) | grid_skip...);
	}(make_integer_sequence<LengthType, DimensionType::length()> {})
		| DisRegRep::Core::View::Functional::MakeFromTuple<DimensionType>;
};

constexpr auto MakeGridRange = []<OffsetRangeMaker Mk>(
	const convertible_to<const Regionfield* const> auto rf,
	const GridStrideType skip,
	const ScalarType grid_size,
	Mk&& offset_range_maker,
	const BitPerSampleResult::BitType group_size
) static constexpr noexcept -> view auto {
	using DimensionType = DiamondSquare::DimensionType;
	//This is a little tick to modify regionfield extent to mimic the effect of dropping the last grids in each rank,
	//	whose sizes are smaller than the specified grid size.
	//To be specific, those grids are clipped by the regionfield matrix border,
	//	leaving only the first row and column, or the top-left corner element for the bottom-right corner of the regionfield.
	//And this is where the **1** in `grid size - 1` comes from, to manually remove the clipped rows and columns.
	const DimensionType clipped_extent = rf->extent() - (grid_size - ScalarType { 1 });
	return std::invoke(std::forward<Mk>(offset_range_maker), clipped_extent, skip)
		| chunk(group_size)
		| transform([rf_2d = rf->range2d(), grid_extent = DimensionType(grid_size)](
			const auto group_offset) constexpr noexcept {
			return tuple(
				group_offset.front(),
				group_offset | transform([&](const auto offset) constexpr noexcept {
					return rf_2d | DisRegRep::Core::View::Matrix::Slice2d(offset, grid_extent);
				})
			);
		});
};
constexpr auto MakePackableGridRange = []<
	typename GridMk,
	const BitPerSampleResult& BpsResult,
	OffsetRangeMaker OffsetMk
>(
	GridMk&& packable_grid_range_maker,
	BpsResultConstant<BpsResult>,
	OffsetMk&& offset_range_maker
) static constexpr noexcept -> view auto {
	return apply([offset_maker = std::forward<OffsetMk>(offset_range_maker)]<typename... Mk>(Mk&&... maker) constexpr noexcept {
		return zip(std::invoke(std::forward<Mk>(maker), offset_maker, BpsResult.PackingFactor)...);
	}, std::forward<GridMk>(packable_grid_range_maker));
};

template<const BitPerSampleResult& PassBps, typename ExecutionPolicy, typename F>
requires is_execution_policy_v<ExecutionPolicy>
void step(const ExecutionPolicy policy, GridInputOutputRange auto&& grid_io, const SecretView secret, F&& f,
	const DisRegRep::Core::XXHash::Hashable auto& salt) {
	using std::ranges::cbegin, std::ranges::cend, std::ranges::size;

	std::for_each(policy, cbegin(grid_io), cend(grid_io), [secret, &f, &salt](auto grid) {
		auto [grid_in_enum, grid_out_enum] = std::move(grid);
		const auto [grid_in_offset, grid_in] = std::move(grid_in_enum);
		const auto [grid_out_offset, grid_out] = std::move(grid_out_enum);
		assert(size(grid_in) <= PassBps.PackingFactor);
		assert(size(grid_out) <= PassBps.PackingFactor);

		for (const HashType hash = DisRegRep::Core::XXHash::hash(secret, tuple(std::cref(salt), grid_in_offset, grid_out_offset));
			auto zipped : zip(
				DisRegRep::Core::Bit::unpack(hash, PassBps.PackingFactor, PassBps)
					| transform([](const auto sample) static constexpr noexcept { return bitset<PassBps.Bit>(sample); }),
				grid_in,
				grid_out
		)) [[likely]] {
			apply([&f](auto... v) { std::invoke(std::forward<F>(f), std::move(v)...); }, std::move(zipped));
		}
	});
}

template<typename... V, typename Array = array<common_type_t<V...>, sizeof...(V)>>
requires conjunction_v<is_copy_constructible<V>...>
[[nodiscard]] constexpr typename Array::value_type choose(const typename Array::size_type sample, const V... value)
	noexcept(conjunction_v<is_nothrow_copy_constructible<V>...>) {
	const Array value_array { value... };
	return value_array[sample];
}

constexpr void copyHalo(const Regionfield& input, Regionfield& output) noexcept {
	const auto rf_tie = tie(input, output);
	const auto [input_2d, output_2d] =
		apply([](auto&... rf) static constexpr noexcept { return tuple(rf.range2d()...); }, rf_tie);
	const auto [input_2d_t, output_2d_t] =
		apply([](auto&... rf) static constexpr noexcept { return tuple(rf.rangeTransposed2d()...); }, rf_tie);

	/**
	 * Copies are ordered as follows to maximise cache coalescence:
	 * | ----> |
	 * |       |
	 * |       |
	 * v ----> v
	 */
	using std::copy_n,
		std::ranges::cbegin, std::ranges::begin, std::ranges::size, std::ranges::range_size_t;
	static constexpr auto copy_size = []<typename In>(
										  const In& r_in, const auto& r_out) static constexpr noexcept -> range_size_t<In> {
		const auto n = size(r_in);
		assert(n == size(r_out));
		return n;
	};
	static constexpr auto copy_vertical = []<typename In, typename Out>(In&& r_in, Out&& r_out) static constexpr noexcept -> void {
		const auto n = copy_size(r_in, r_out);
		copy_n(unseq, cbegin(std::forward<In>(r_in)), n, begin(std::forward<Out>(r_out)));
	};
	static constexpr auto copy_horizontal = []<typename In, typename Out>(In&& r_in, Out&& r_out) static constexpr noexcept -> void {
		const auto n = copy_size(r_in, r_out);
		assert(n >= 2U);
		copy_n(unseq, ++cbegin(std::forward<In>(r_in)), n - 2U, ++begin(std::forward<Out>(r_out)));
	};
	//Left
	copy_vertical(input_2d.front(), output_2d.front());
	//Top
	copy_horizontal(input_2d_t.front(), output_2d_t.front());
	//Bottom
	copy_horizontal(input_2d_t.back(), output_2d_t.back());
	//Right
	copy_vertical(input_2d.back(), output_2d.back());
}

/**
 * In each iteration:
 * Input
 * | -- | -- |
 * | nw | ne |
 * | -- | -- |
 * | sw | se |
 * | -- | -- |
 * Output:
 * | -------- | -------------------- | -------- |
 * |    nw    |       nw OR ne       |    ne    |
 * | -------- | -------------------- | -------- |
 * | nw OR sw | nw OR ne OR sw OR se | ne OR se |
 * | -------- | -------------------- | -------- |
 * |    sw    |       sw OR se       |    se    |
 * | -------- | -------------------- | -------- |
 * Where *OR* denotes a uniform-random selection among all specified values.
 *
 * To parallelise, the Diamond-Square algorithm needs to be decomposed into two separate passes
 *	due to overlapping output grids, which may cause data racing.
 * It basically breaks the overlapping grid elements into independent runs.
 */
template<typename ExecutionPolicy>
requires is_execution_policy_v<ExecutionPolicy>
void upscale(const ExecutionPolicy policy, const Regionfield& input, Regionfield& output, const SecretView secret) {
	using DimensionType = DiamondSquare::DimensionType;
	const auto make_grid_step_range = bind_front(MakePackableGridRange, tuple(
		bind_front(MakeGridRange, &input, 1U, 2U),
		bind_front(MakeGridRange, &output, 2U, 3U)
	));
	//The first pass computes the 2x2 sub-grid from the top-left corner for the entire domain.
	step<FirstPassBps>(policy,
		make_grid_step_range(BpsResultConstant<FirstPassBps> {}, MakeOffsetRange),
		secret,
		[](const auto sample, const auto in, const auto out) static constexpr noexcept {
			const Regionfield::ValueType
				nw = in[0][0],
				sw = in[0][1],
				ne = in[1][0],
				se = in[1][1];

			out[0][0] = nw;
			out[0][1] = choose(sample[0], nw, sw);
			out[1][0] = choose(sample[1], nw, ne);
			out[1][1] = choose((sample >> 2U).to_ulong(), nw, sw, ne, se);
		},
		FirstPassSalt
	);
	//The second pass computes the remaining sub-grid (bottom two plus the right-most column)
	//	whose domain has the same size as the sub-grid.
	step<SecondPassBps>(unseq, make_grid_step_range(BpsResultConstant<SecondPassBps> {},
		[](const DimensionType extent, const GridStrideType skip) static constexpr noexcept {
			const auto grid_skip = stride(skip);
			return DisRegRep::Core::View::Concat(
				iota(ScalarType {}, extent.x - 1U)
					| grid_skip
					| transform([y = extent.y - 1U](const auto x) constexpr noexcept { return DimensionType(x, y); }),
				iota(ScalarType {}, extent.y)
					| grid_skip
					| transform([x = extent.x - 1U](const auto y) constexpr noexcept { return DimensionType(x, y); })
			);
		}),
		secret,
		[](const auto sample, const auto in, const auto out) static constexpr noexcept {
			const Regionfield::ValueType
				sw = in[0][1],
				ne = in[1][0],
				se = in[1][1];

			out[0][2] = sw;
			out[1][2] = choose(sample[0], sw, se);

			out[2][0] = ne;
			out[2][1] = choose(sample[1], ne, se);
			out[2][2] = se;
		},
		SecondPassSalt
	);
}

/**
 * Smoothing pass is mainly to reduce output randomness to improve its quality.
 * Input:
 * | - | - | - |
 * |   | n |   |
 * | - | - | - |
 * | w | c | e |
 * | - | - | - |
 * |   | s |   |
 * | - | - | - |
 * Output: Modify `c` based on the following procedure:
 * - If all opposite cells equal: choose randomly among { n, e, s, w }.
 * - If the horizontal/vertical cell equals: choose randomly between { w, e }/{ n, w }.
 * - Unchanged.
 */
template<typename ExecutionPolicy>
requires is_execution_policy_v<ExecutionPolicy>
void smooth(const ExecutionPolicy policy, const Regionfield& input, Regionfield& output, const SecretView secret) {
	using DimensionType = DiamondSquare::DimensionType;
	step<SmoothPassBps>(policy, zip(
			MakeGridRange(&input, 1U, 3U, MakeOffsetRange, SmoothPassBps.PackingFactor),
			MakeGridRange(&output, 1U, 1U,
				bind_back(MakeOffsetRange, DimensionType(1U), DimensionType(2U)), SmoothPassBps.PackingFactor)
		),
		secret,
		[](const auto sample, const auto in, const auto out_singular) static constexpr noexcept {
			const Regionfield::ValueType
				c = in[1][1],
				n = in[1][0],
				e = in[2][1],
				s = in[1][2],
				w = in[0][1];
			const bool
				eq_horizontal = e == w,
				eq_vertical = n == s;

			if (Regionfield::ValueType& out = out_singular.front().front();
				eq_horizontal && eq_vertical) [[likely]] {
				//It is more likely value in all directions are equal than just value in opposite directions.
				out = choose(sample.to_ulong(), n, e);
			} else if (eq_horizontal) {
				out = e;
			} else if (eq_vertical) {
				out = n;
			} else [[unlikely]] {
				out = c;
			}
		},
		SmoothPassSalt
	);
}

template<typename ExecutionPolicy>
requires is_execution_policy_v<ExecutionPolicy>
void resize(const ExecutionPolicy policy, const Regionfield& input, Regionfield& output) {
	using DimensionType = DiamondSquare::DimensionType;
	const auto [ext_in, ext_out] =
		apply([](const auto&... rf) static constexpr noexcept { return tuple(rf.extent()...); }, tie(input, output));

	using glm::f32vec2;
	const auto index = MakeOffsetRange(ext_out, 1U);
	std::transform(policy, index.cbegin(), index.cend(), output.span().begin(),
		[max_idx_in = f32vec2(ext_in - 1U), max_idx_out = f32vec2(ext_out - 1U), in = input.mdspan()](
			const auto idx_out) noexcept {
			const DimensionType idx_in = glm::round(f32vec2(idx_out) / max_idx_out * max_idx_in);
			return in[idx_in.x, idx_in.y];
		});
}

}

DRR_REGIONFIELD_GENERATOR_DEFINE_DELEGATING_FUNCTOR(DiamondSquare) {
	const auto iteration_count = this->Iteration.size();
	DRR_ASSERT(glm::all(greaterThanEqual(this->InitialExtent, DimensionType(2U))));
	DRR_ASSERT(iteration_count > 0U);

	const DimensionType final_extent = fold_left(repeat(std::uint_least8_t {}, iteration_count), this->InitialExtent,
		[](const auto dim, auto) static constexpr noexcept { return upscale(dim); }),
		output_extent = regionfield.extent();

	this->PingPong.RegionCount = regionfield.RegionCount;
	this->PingPong.reserve(final_extent);

	const Regionfield* input = &this->PingPong;
	Regionfield* output = &regionfield;
	const auto swap_buffer = [&] constexpr noexcept {
		std::swap(const_cast<Regionfield*&>(input), output);//NOLINT(cppcoreguidelines-pro-type-const-cast)
	};
	if (!(fold_left(this->Iteration, iteration_count, plus {}) & 1U)) {
		//This is to correct the initial regionfield buffer order
		//	to ensure the user-provided regionfield is the output after the last iteration.
		//If the output needs to be resized, an extra iteration is required after stepping through every iteration.
		swap_buffer();
	}
	{
		static constexpr Uniform InitialGridGenerator;
		auto& init_input = const_cast<Regionfield&>(*input);//NOLINT(cppcoreguidelines-pro-type-const-cast)
		init_input.resize(this->InitialExtent);
		InitialGridGenerator(ExecutionPolicy::SingleThreadingTrait, init_input, gen_info);
	}
	for (const Core::XXHash::Secret secret = DiamondSquare::generateSecret(gen_info);
		const auto smoothing_iteration : this->Iteration) [[likely]] {
		output->resize(upscale(input->extent()));
		upscale(EpTrait::Unsequenced, *input, *output, secret);

		swap_buffer();
		if (smoothing_iteration == 0U) {
			continue;
		}

		output->resize(input->extent());
		copyHalo(*input, *output);
		for (const auto _ : repeat(std::uint_least8_t {}, smoothing_iteration)) [[likely]] {
			smooth(EpTrait::Unsequenced, *input, *output, secret);
			swap_buffer();
		}
	}
	//Undo the last buffer swap since the output already points us to the user-provided regionfield.
	swap_buffer();
	assert(output == &regionfield);

	if (final_extent != output_extent) {
		output->resize(output_extent);
		resize(EpTrait::Unsequenced, *input, *output);
	}
}

DRR_REGIONFIELD_GENERATOR_DEFINE_FUNCTOR_ALL(DiamondSquare)