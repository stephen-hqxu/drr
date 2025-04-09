#include <DisRegRep/Splatting/OccupancyConvolution/Full/Vanilla.hpp>
#include <DisRegRep/Splatting/ImplementationHelper.hpp>

#include <DisRegRep/Container/SplatKernel.hpp>

#include <DisRegRep/Core/View/Trait.hpp>

#include <tuple>

#include <algorithm>
#include <ranges>

#include <utility>

using DisRegRep::Splatting::OccupancyConvolution::Full::Vanilla;

using std::tie, std::apply;

namespace {

DRR_SPLATTING_DEFINE_SCRATCH_MEMORY {
public:

	DRR_SPLATTING_SCRATCH_MEMORY_CONTAINER_TRAIT;

	using ExtentType = typename ContainerTrait::MaskOutputType::Dimension3Type;

	typename ContainerTrait::KernelType Kernel;
	typename ContainerTrait::MaskOutputType Output;

	//(width, height, region count)
	void resize(const ExtentType extent) {
		this->Kernel.resize(extent.z);
		this->Output.resize(extent);
	}

	[[nodiscard]] Vanilla::SizeType sizeByte() const noexcept {
		return apply([](const auto&... member) static noexcept { return (member.sizeByte() + ...); }, tie(this->Kernel, this->Output));
	}

};

}

DRR_SPLATTING_DEFINE_DELEGATING_FUNCTOR(Vanilla) {
	this->validate(regionfield, invoke_info);
	auto& [kernel_memory, output_memory] = ImplementationHelper::allocate<ScratchMemory, ContainerTrait>(
		memory, typename ScratchMemory<ContainerTrait>::ExtentType(invoke_info.Extent, regionfield.RegionCount));

	using std::ranges::transform, std::ranges::for_each;
	transform(this->convolve(invoke_info, regionfield), output_memory.range().begin(),
		[&kernel_memory, norm_factor = this->kernelNormalisationFactor()]<typename Kernel>(Kernel&& kernel) noexcept(
			Core::View::Trait::IsNothrowViewable<Kernel>) {
			kernel_memory.clear();
			for_each(std::forward<Kernel>(kernel) | std::views::join,
				[&kernel_memory](const auto region_id) noexcept { kernel_memory.increment(region_id); });
			return DisRegRep::Container::SplatKernel::toMask(kernel_memory, norm_factor);
		});

	return output_memory;
}

DRR_SPLATTING_DEFINE_SIZE_BYTE(Vanilla)
DRR_SPLATTING_DEFINE_FUNCTOR_ALL(Vanilla)