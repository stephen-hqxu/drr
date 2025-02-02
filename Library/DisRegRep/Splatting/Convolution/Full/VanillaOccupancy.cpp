#include <DisRegRep/Splatting/Convolution/Full/VanillaOccupancy.hpp>
#include <DisRegRep/Splatting/Convolution/Base.hpp>
#include <DisRegRep/Splatting/ImplementationHelper.hpp>

#include <DisRegRep/Container/SplatKernel.hpp>

#include <DisRegRep/Core/View/Matrix.hpp>

#include <tuple>

#include <algorithm>
#include <ranges>

#include <cstddef>

using DisRegRep::Splatting::Convolution::Full::VanillaOccupancy;

using std::tie, std::apply;
using std::views::cartesian_product, std::views::iota, std::views::transform,
	std::views::take, std::views::join;
using std::index_sequence;

namespace {

DRR_SPLATTING_DEFINE_SCRATCH_MEMORY {
public:

	DRR_SPLATTING_SCRATCH_MEMORY_CONTAINER_TRAIT;

	using ExtentType = typename ContainerTrait::MaskOutputType::Dimension3Type;

	typename ContainerTrait::KernelType Kernel;
	typename ContainerTrait::MaskOutputType Output;

	//(region count, width, height)
	void resize(const ExtentType extent) {
		this->Kernel.resize(extent.x);
		this->Output.resize(extent);
	}

	[[nodiscard]] VanillaOccupancy::SizeType sizeByte() const noexcept {
		return apply([](const auto&... member) static noexcept { return (member.sizeByte() + ...); }, tie(this->Kernel, this->Output));
	}

};

}

DRR_SPLATTING_DEFINE_DELEGATING_FUNCTOR(VanillaOccupancy) {
	this->validate(regionfield, info);

	using ScratchMemoryType = ScratchMemory<ContainerTrait>;
	const auto [offset, extent] = info;
	auto& [kernel_memory, output_memory] = ImplementationHelper::allocate<ScratchMemory, ContainerTrait>(
		memory, typename ScratchMemoryType::ExtentType(regionfield.RegionCount, extent));

	const KernelSizeType d = Convolution::Base::diametre(this->Radius);

	const auto element_rg = [r = this->Radius, &offset, &extent]<std::size_t... I>(index_sequence<I...>) constexpr noexcept {
		return cartesian_product(iota(offset[I] - r) | take(extent[I])...);
	}(index_sequence<1U, 0U> {});
	const auto kernel_rg = element_rg
		| transform([d, rf_2d = regionfield.range2d()](const auto idx) constexpr noexcept {
			const auto [y, x] = idx;
			return rf_2d
				| Core::View::Matrix::SubRange2d(DimensionType(x, y), DimensionType(d))
				| join;
		  });

	using std::ranges::transform, std::ranges::for_each;
	transform(kernel_rg, output_memory.range().begin(),
		[&kernel_memory, norm_factor = Base::kernelNormalisationFactor(d)](auto kernel) noexcept {
			kernel_memory.clear();
			for_each(kernel, [&kernel_memory](const auto region_id) noexcept { kernel_memory.increment(region_id); });
			return Container::SplatKernel::toMask(kernel_memory, norm_factor);
		});

	return output_memory;
}

DRR_SPLATTING_DEFINE_SIZE_BYTE(VanillaOccupancy)
DRR_SPLATTING_DEFINE_FUNCTOR_ALL(VanillaOccupancy)