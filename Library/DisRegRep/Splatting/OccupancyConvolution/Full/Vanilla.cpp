#include <DisRegRep/Splatting/OccupancyConvolution/Full/Vanilla.hpp>
#include <DisRegRep/Splatting/ImplementationHelper.hpp>

#include <DisRegRep/Container/SplatKernel.hpp>

#include <algorithm>
#include <ranges>

#include <utility>

using DisRegRep::Splatting::OccupancyConvolution::Full::Vanilla,
	DisRegRep::Splatting::ImplementationHelper::PredefinedScratchMemory::Simple;

using std::ranges::transform, std::ranges::for_each;

DRR_SPLATTING_DEFINE_DELEGATING_FUNCTOR(Vanilla) {
	this->validate(invoke_info, regionfield);
	auto& [kernel_memory, output_memory] =
		ImplementationHelper::PredefinedScratchMemory::allocateSimple<ContainerTrait>(invoke_info, regionfield, memory);

	transform(this->convolve(invoke_info, regionfield), output_memory.range().begin(),
		[&kernel_memory, norm_factor = this->area()]<typename Kernel>(Kernel&& kernel) noexcept {
			kernel_memory.clear();
			for_each(std::forward<Kernel>(kernel) | std::views::join,
				[&kernel_memory](const auto region_id) noexcept { kernel_memory.increment(region_id); });
			return DisRegRep::Container::SplatKernel::toMask(kernel_memory, norm_factor);
		});
	return output_memory;
}

DRR_SPLATTING_DEFINE_SIZE_BYTE(Vanilla, Simple)
DRR_SPLATTING_DEFINE_FUNCTOR_ALL(Vanilla)