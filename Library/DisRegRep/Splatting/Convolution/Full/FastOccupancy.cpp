#include <DisRegRep/Splatting/Convolution/Full/FastOccupancy.hpp>
#include <DisRegRep/Splatting/Convolution/Base.hpp>
#include <DisRegRep/Splatting/ImplementationHelper.hpp>

#include <DisRegRep/Container/SplatKernel.hpp>
#include <DisRegRep/Container/SplattingCoefficient.hpp>

#include <DisRegRep/Core/Arithmetic.hpp>

#include <tuple>

#include <algorithm>
#include <functional>
#include <iterator>
#include <ranges>

#include <utility>

#include <concepts>
#include <type_traits>

#include <cassert>

namespace SpltKn = DisRegRep::Container::SplatKernel;
using DisRegRep::Splatting::Convolution::Full::FastOccupancy;

using std::tuple, std::make_tuple, std::tie, std::apply;
using std::ranges::for_each,
	std::views::take, std::views::drop, std::views::zip;
using std::invoke, std::identity;

using std::output_iterator;
using std::ranges::forward_range,
	std::ranges::range_difference_t, std::ranges::range_value_t, std::ranges::range_const_reference_t;
using std::invocable, std::invoke_result_t;

namespace {

DRR_SPLATTING_DEFINE_SCRATCH_MEMORY {
public:

	DRR_SPLATTING_SCRATCH_MEMORY_CONTAINER_TRAIT;

	using ExtentType = DisRegRep::Container::SplattingCoefficient::Type::Dimension3Type;

	typename ContainerTrait::KernelType Kernel;
	typename ContainerTrait::ImportanceOutputType Horizontal;
	typename ContainerTrait::MaskOutputType Vertical;

	//(region count, width, height)
	void resize(const tuple<ExtentType, FastOccupancy::KernelSizeType> arg) {
		auto [extent, padding] = arg;
		this->Kernel.resize(extent.x);
		this->Vertical.resize(extent);
		extent.z += padding;
		this->Horizontal.resize(extent);
	}

	[[nodiscard]] FastOccupancy::SizeType sizeByte() const noexcept {
		return apply([](const auto&... member) static noexcept { return (member.sizeByte() + ...); },
			tie(this->Kernel, this->Horizontal, this->Vertical));
	}

};

template<
	forward_range ScanlineRange,
	SpltKn::Is KernelMemory,
	typename KernelMemoryProj,
	forward_range Scanline = range_value_t<ScanlineRange>
>
void conv1d(
	const ScanlineRange scanline_rg,
	KernelMemory& kernel_memory,
	output_iterator<invoke_result_t<KernelMemoryProj, const KernelMemory&>> auto out,
	const range_difference_t<Scanline> d,
	KernelMemoryProj kernel_memory_proj,
	invocable<range_const_reference_t<Scanline>> auto scanline_element_proj
) {
	for (const Scanline scanline : scanline_rg) [[likely]] {
		kernel_memory.clear();

		//Compute the initial kernel in this scanline.
		for_each(scanline | take(d), [&kernel_memory, &proj = scanline_element_proj](
										 const auto element) noexcept { kernel_memory.increment(invoke(proj, element)); });
		*out++ = invoke(kernel_memory_proj, std::as_const(kernel_memory));

		//Kernel sliding.
		using std::ranges::transform;
		//First element from the current kernel.
		//The good thing is we don't need to calculate the size of decrement range and simply rely on zip view,
		//	and we can avoid getting the size of the scanline and do annoying maths.
		const auto decrement_rg = scanline;
		//Last element from the next kernel.
		const auto increment_rg = scanline | drop(d);
		out = transform(zip(decrement_rg, increment_rg), out,
			[&kernel_memory, &se_proj = scanline_element_proj, &km_proj = kernel_memory_proj](const auto it) noexcept {
				const auto& [dec_element, inc_element] = it;
				//Decrement first is slightly more efficient,
				//	in case of sparse kernel, it will need to remove empty elements and shift following elements ahead.
				//Increment inserts at the back.
				kernel_memory.decrement(invoke(se_proj, dec_element));
				kernel_memory.increment(invoke(se_proj, inc_element));
				return invoke(km_proj, std::as_const(kernel_memory));
			}).out;
	}
}

}

DRR_SPLATTING_DEFINE_DELEGATING_FUNCTOR(FastOccupancy) {
	this->validate(regionfield, info);

	using ScratchMemoryType = ScratchMemory<ContainerTrait>;
	const auto [offset, extent] = info;

	const KernelSizeType d = Convolution::Base::diametre(this->Radius),
		//Padding does not include the centre element (only the halo), so minus one from the diametre.
		d_halo = d - 1U;

	//Horizontal pass requires padding above and below the matrix.
	auto& [kernel_memory, horizontal_memory, vertical_memory] = ImplementationHelper::allocate<ScratchMemory, ContainerTrait>(
		memory, make_tuple(typename ScratchMemoryType::ExtentType(regionfield.RegionCount, extent), d_halo));

	//Need to read the whole halo from regionfield.
	//In horizontal scanline, this overlaps with the 1D kernel.
	//In vertical scanline, this includes the padding.
	conv1d(
		regionfield.range2d() | Core::Arithmetic::SubRange2d(offset - this->Radius, extent + d_halo),
		kernel_memory,
		horizontal_memory.range().begin(),
		d,
		[](const auto& km) static constexpr noexcept { return km.span(); },
		identity {}
	);

	//Repeat the same process in the vertical pass.
	conv1d(
		horizontal_memory.rangeTransposed2d(),
		kernel_memory,
		vertical_memory.range().begin(),
		d,
		[norm_factor = Base::kernelNormalisationFactor(d)](
			const auto& km) constexpr noexcept { return SpltKn::toMask(km, norm_factor); },
		[](const auto& proxy) static constexpr noexcept { return *proxy; }
	);

	return vertical_memory;
}

DRR_SPLATTING_DEFINE_SIZE_BYTE(FastOccupancy)
DRR_SPLATTING_DEFINE_FUNCTOR_ALL(FastOccupancy)