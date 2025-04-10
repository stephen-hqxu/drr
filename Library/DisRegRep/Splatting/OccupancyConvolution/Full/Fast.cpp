#include <DisRegRep/Splatting/OccupancyConvolution/Full/Fast.hpp>
#include <DisRegRep/Splatting/OccupancyConvolution/Base.hpp>
#include <DisRegRep/Splatting/ImplementationHelper.hpp>

#include <DisRegRep/Container/SplatKernel.hpp>
#include <DisRegRep/Container/SplattingCoefficient.hpp>

#include <DisRegRep/Core/View/Functional.hpp>
#include <DisRegRep/Core/View/Matrix.hpp>
#include <DisRegRep/Core/MdSpan.hpp>

#include <tuple>

#include <algorithm>
#include <functional>
#include <iterator>
#include <ranges>

#include <utility>

#include <concepts>
#include <type_traits>

namespace SpltKn = DisRegRep::Container::SplatKernel;
using DisRegRep::Splatting::OccupancyConvolution::Full::Fast;

using std::tuple, std::tie, std::apply;
using std::ranges::for_each,
	std::bind_back, std::bit_or, std::invoke,
	std::views::take, std::views::drop, std::views::zip, std::views::transform;
using std::output_iterator,
	std::ranges::forward_range, std::ranges::view,
	std::ranges::range_difference_t, std::ranges::range_value_t, std::ranges::range_const_reference_t;
using std::invocable, std::invoke_result_t, std::common_type_t;

namespace {

DRR_SPLATTING_DEFINE_SCRATCH_MEMORY(ScratchMemory) {
public:

	DRR_SPLATTING_SCRATCH_MEMORY_CONTAINER_TRAIT;

	using ExtentType = common_type_t<
		typename ContainerTrait::ImportanceOutputType::Dimension3Type,
		typename ContainerTrait::MaskOutputType::Dimension3Type
	>;

	typename ContainerTrait::KernelType Kernel;
	typename ContainerTrait::ImportanceOutputType Vertical;
	typename ContainerTrait::MaskOutputType Horizontal;

	//(width, height, region count)
	void resize(const tuple<ExtentType, Fast::KernelSizeType> arg) {
		using DisRegRep::Core::MdSpan::reverse;
		auto [extent, padding] = arg;

		this->Kernel.resize(extent.z);
		//The final mask output will be a transposed of the input regionfield, so we flip the dense axes.
		this->Horizontal.resize(ExtentType(reverse(typename ContainerTrait::MaskOutputType::Dimension2Type(extent)), extent.z));
		extent.x += padding;
		this->Vertical.resize(extent);
	}

	[[nodiscard]] Fast::SizeType sizeByte() const noexcept {
		return apply([](const auto&... member) static noexcept { return (member.sizeByte() + ...); },
			tie(this->Kernel, this->Vertical, this->Horizontal));
	}

};

template<
	forward_range ScanlineRange,
	SpltKn::Is KernelMemory,
	typename KernelMemoryProj,
	forward_range Scanline = range_value_t<ScanlineRange>
>
requires view<Scanline>
void conv1d(
	ScanlineRange&& scanline_rg,
	KernelMemory& kernel_memory,
	output_iterator<invoke_result_t<KernelMemoryProj, const KernelMemory&>> auto out,
	const range_difference_t<Scanline> d,
	KernelMemoryProj kernel_memory_proj
) {
	for (const auto scanline : std::forward<ScanlineRange>(scanline_rg)) [[likely]] {
		kernel_memory.clear();

		//Compute the initial kernel in this scanline.
		for_each(scanline | take(d), [&kernel_memory](const auto element) noexcept { kernel_memory.increment(element); });
		*out++ = invoke(kernel_memory_proj, std::as_const(kernel_memory));

		//Kernel sliding.
		using std::ranges::transform;
		//First element from the current kernel.
		//The good thing is we don't need to calculate the size of decrement range and simply rely on zip view,
		//	and we can avoid getting the size of the scanline and do annoying maths.
		const auto decrement_rg = scanline;
		//Last element from the next kernel.
		const auto increment_rg = scanline | drop(d);
		out = transform(
			zip(decrement_rg, increment_rg),
			std::move(out),
			[&kernel_memory, &km_proj = kernel_memory_proj](const auto it) noexcept {
				const auto& [dec_element, inc_element] = it;
				//Decrement first is slightly more efficient,
				//	in case of sparse kernel, it will need to remove empty elements and shift following elements ahead.
				//Increment inserts at the back.
				kernel_memory.decrement(dec_element);
				kernel_memory.increment(inc_element);
				return invoke(km_proj, std::as_const(kernel_memory));
			}
		).out;
	}
}

}

DRR_SPLATTING_DEFINE_DELEGATING_FUNCTOR(Fast) {
	this->validate(invoke_info, regionfield);
	const auto [offset, extent] = invoke_info;
	const KernelSizeType d = this->diametre(),
		//Padding does not include the centre element (only the halo), so minus one from the diametre.
		d_halo = d - 1U;
	//Vertical pass requires padding to the left and right of the matrix.
	auto& [kernel_memory, vertical_memory, horizontal_memory] = ImplementationHelper::allocate<ScratchMemory, ContainerTrait>(
		memory, tuple(typename ScratchMemory<ContainerTrait>::ExtentType(extent, regionfield.RegionCount), d_halo));

	//Need to read the whole halo from regionfield.
	//In vertical scanline, this overlaps with the 1D kernel.
	//In horizontal scanline, this includes the padding.
	conv1d(
		regionfield.range2d() | Core::View::Matrix::Slice2d(offset - this->Radius, extent + d_halo),
		kernel_memory,
		vertical_memory.range().begin(),
		d,
		[](const auto& km) static constexpr noexcept { return km.span(); }
	);
	//Repeat the same process in the horizontal pass.
	conv1d(
		vertical_memory.rangeTransposed2d() | transform(bind_back(bit_or {}, Core::View::Functional::Dereference)),
		kernel_memory,
		horizontal_memory.range().begin(),
		d,
		[norm_factor = Base::kernelNormalisationFactor(d)](
			const auto& km) constexpr noexcept { return SpltKn::toMask(km, norm_factor); }
	);

	return horizontal_memory;
}

DRR_SPLATTING_DEFINE_SIZE_BYTE(Fast, ScratchMemory)
DRR_SPLATTING_DEFINE_FUNCTOR_ALL(Fast)