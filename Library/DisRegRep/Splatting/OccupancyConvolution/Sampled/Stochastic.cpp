#include <DisRegRep/Splatting/OccupancyConvolution/Sampled/Stochastic.hpp>
#include <DisRegRep/Splatting/OccupancyConvolution/Sampled/Base.hpp>
#include <DisRegRep/Splatting/ImplementationHelper.hpp>

#include <DisRegRep/Container/Regionfield.hpp>
#include <DisRegRep/Container/SplatKernel.hpp>

#include <DisRegRep/Core/Exception.hpp>
#include <DisRegRep/Core/XXHash.hpp>

#include <algorithm>
#include <ranges>

#include <random>
#include <utility>

#include <cstdint>

using DisRegRep::Splatting::OccupancyConvolution::Sampled::Stochastic,
	DisRegRep::Splatting::ImplementationHelper::PredefinedScratchMemory::Simple,
	DisRegRep::Container::Regionfield;

using std::ranges::transform, std::ranges::for_each,
	std::views::repeat;
using std::uniform_int_distribution,
	std::integer_sequence, std::make_integer_sequence;

DRR_SPLATTING_DEFINE_DELEGATING_FUNCTOR(Stochastic) {
	this->validate(invoke_info, regionfield);
	auto& [kernel_memory, output_memory] =
		ImplementationHelper::PredefinedScratchMemory::allocateSimple<ContainerTrait>(invoke_info, regionfield, memory);

	transform(this->convolve(Stochastic::IncludeOffsetEnumeration, invoke_info, regionfield), output_memory.range().begin(),
		[
			&kernel_memory,
			secret = Stochastic::generateSecret(this->Seed),
			sample_dist = uniform_int_distribution<KernelSizeType>(0U, this->diametre() - 1U),
			sample_repetition = repeat(std::uint_least8_t {}, this->Sample),
			norm_factor = this->Sample
		](auto offset_kernel) mutable {
			auto [offset, kernel] = std::move(offset_kernel);

			kernel_memory.clear();
			for_each(sample_repetition,
				[
					&kernel_memory,
					&sample_dist,
					kernel = std::move(kernel),
					rng = Core::XXHash::RandomEngine(secret, auto(offset))
				](auto) mutable {
					using LengthType = DimensionType::length_type;
					//It is too expensive to shuffle an index sequence than just taking some random samples from the kernel,
					//	although that can avoid taking duplicate samples.
					const DimensionType idx = [&]<LengthType... I>(integer_sequence<LengthType, I...>) {
						return DimensionType(((void)I, sample_dist(rng))...);
					}(make_integer_sequence<LengthType, DimensionType::length()> {});

					kernel_memory.increment(kernel[idx.x][idx.y]);
				});
			return DisRegRep::Container::SplatKernel::toMask(kernel_memory, norm_factor);
		});
	return output_memory;
}

void Stochastic::validate(const InvokeInfo& invoke_info, const Regionfield& regionfield) const {
	this->Base::validate(invoke_info, regionfield);

	DRR_ASSERT(this->Sample > 0U);
}

DRR_SPLATTING_DEFINE_SIZE_BYTE(Stochastic, Simple)
DRR_SPLATTING_DEFINE_FUNCTOR_ALL(Stochastic)