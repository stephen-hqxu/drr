#include <DisRegRep/Splatting/OccupancyConvolution/Full/Vanilla.hpp>
#include <DisRegRep/Splatting/OccupancyConvolution/Base.hpp>
#include <DisRegRep/Splatting/ImplementationHelper.hpp>

#include <DisRegRep/Container/SplatKernel.hpp>

#include <DisRegRep/Core/View/Functional.hpp>
#include <DisRegRep/Core/View/Matrix.hpp>

#include <tuple>

#include <algorithm>
#include <ranges>

#include <utility>

using DisRegRep::Splatting::OccupancyConvolution::Full::Vanilla;

using std::tie, std::apply;
using std::views::cartesian_product, std::views::iota, std::views::transform, std::views::join;
using std::integer_sequence, std::make_integer_sequence;

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
	this->validate(regionfield, info);

	using ScratchMemoryType = ScratchMemory<ContainerTrait>;
	const auto [offset, extent] = info;
	auto& [kernel_memory, output_memory] = ImplementationHelper::allocate<ScratchMemory, ContainerTrait>(
		memory, typename ScratchMemoryType::ExtentType(extent, regionfield.RegionCount));

	const KernelSizeType d = OccupancyConvolution::Base::diametre(this->Radius);

	using LengthType = DimensionType::length_type;
	const auto element_rg = [r = this->Radius, &offset, &extent]<LengthType... I>(
								integer_sequence<LengthType, I...>) constexpr noexcept {
		//It is much more clean to use std::views::take, but I want to keep iota_view sized to allow better compiler optimisation.
		return cartesian_product([&, r] constexpr noexcept {
			const DimensionType::value_type start = offset[I] - r;
			return iota(start, start + extent[I]);
		}()...) | Core::View::Functional::MakeFromTuple<DimensionType>;
	}(make_integer_sequence<LengthType, DimensionType::length()> {});
	const auto kernel_rg = element_rg
		| transform([d, rf_2d = regionfield.range2d()](const auto idx) constexpr noexcept {
			return rf_2d
				| Core::View::Matrix::Slice2d(idx, DimensionType(d))
				| join;
		});

	using std::ranges::transform, std::ranges::for_each;
	transform(kernel_rg, output_memory.range().begin(),
		[&kernel_memory, norm_factor = Base::kernelNormalisationFactor(d)](auto kernel) noexcept {
			kernel_memory.clear();
			for_each(kernel, [&kernel_memory](const auto region_id) noexcept { kernel_memory.increment(region_id); });
			return DisRegRep::Container::SplatKernel::toMask(kernel_memory, norm_factor);
		});

	return output_memory;
}

DRR_SPLATTING_DEFINE_SIZE_BYTE(Vanilla)
DRR_SPLATTING_DEFINE_FUNCTOR_ALL(Vanilla)