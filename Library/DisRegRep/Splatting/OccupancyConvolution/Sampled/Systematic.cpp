#include <DisRegRep/Splatting/OccupancyConvolution/Sampled/Systematic.hpp>
#include <DisRegRep/Splatting/OccupancyConvolution/Sampled/Base.hpp>
#include <DisRegRep/Splatting/ImplementationHelper.hpp>

#include <DisRegRep/Container/Regionfield.hpp>
#include <DisRegRep/Container/SplatKernel.hpp>

#include <DisRegRep/Core/Exception.hpp>

#include <glm/vector_relational.hpp>

#include <algorithm>
#include <functional>
#include <ranges>

#include <utility>

#include <cassert>

using DisRegRep::Splatting::OccupancyConvolution::Sampled::Systematic,
	DisRegRep::Splatting::ImplementationHelper::PredefinedScratchMemory::Simple,
	DisRegRep::Container::Regionfield;

using std::ranges::for_each, std::ranges::fold_left_first,
	std::bind_back, std::bit_or, std::plus,
	std::views::drop, std::views::stride, std::views::transform, std::views::join;

DRR_SPLATTING_DEFINE_DELEGATING_FUNCTOR(Systematic) {
	this->validate(invoke_info, regionfield);
	auto& [kernel_memory, output_memory] =
		ImplementationHelper::PredefinedScratchMemory::allocateSimple<ContainerTrait>(invoke_info, regionfield, memory);

	const KernelSizeType d = this->diametre();
	const DimensionType remained_size = d - this->FirstSample,
		sample_size = (remained_size + this->Interval - 1U) / this->Interval;

	std::ranges::transform(this->convolve(Systematic::ExcludeOffsetEnumeration, invoke_info, regionfield),
		output_memory.range().begin(),
		[
			this,
			&kernel_memory,
			norm_factor = static_cast<typename decltype(output_memory)::ValueType>(sample_size.x * sample_size.y)
		](auto kernel) noexcept {
			auto kernel_uniform = std::move(kernel)
				| drop(this->FirstSample.x)
				| stride(this->Interval.x)
				| transform(bind_back(bit_or {}, drop(this->FirstSample.y) | stride(this->Interval.y)));
			//It is also possible calculate normalisation factor here with the following method, which is more intuitive.
			//Doing it manually outside the loop to avoid making repetitive calculations.
			assert(fold_left_first(kernel_uniform | transform(std::ranges::size), plus {}) == norm_factor);

			kernel_memory.clear();
			for_each(std::move(kernel_uniform) | join,
				[&kernel_memory](const auto region_id) noexcept { kernel_memory.increment(region_id); });
			return DisRegRep::Container::SplatKernel::toMask(kernel_memory, norm_factor);
		});
	return output_memory;
}

void Systematic::validate(const InvokeInfo& invoke_info, const Regionfield& regionfield) const {
	this->Base::validate(invoke_info, regionfield);

	using glm::all;
	DRR_ASSERT(all(glm::lessThan(this->FirstSample, DimensionType(this->diametre()))));
	DRR_ASSERT(all(glm::greaterThan(this->Interval, DimensionType(0U))));
}

DRR_SPLATTING_DEFINE_SIZE_BYTE(Systematic, Simple)
DRR_SPLATTING_DEFINE_FUNCTOR_ALL(Systematic)