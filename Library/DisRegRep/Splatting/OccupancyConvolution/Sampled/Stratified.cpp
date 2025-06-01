#include <DisRegRep/Splatting/OccupancyConvolution/Sampled/Stratified.hpp>
#include <DisRegRep/Splatting/OccupancyConvolution/Sampled/Base.hpp>
#include <DisRegRep/Splatting/ImplementationHelper.hpp>

#include <DisRegRep/Container/Regionfield.hpp>
#include <DisRegRep/Container/SplatKernel.hpp>

#include <DisRegRep/Core/View/Functional.hpp>
#include <DisRegRep/Core/Exception.hpp>
#include <DisRegRep/Core/XXHash.hpp>

#include <glm/fwd.hpp>
#include <glm/common.hpp>
#include <glm/vec2.hpp>

#include <tuple>

#include <algorithm>
#include <functional>
#include <ranges>

#include <random>

#include <utility>

using DisRegRep::Splatting::OccupancyConvolution::Sampled::Stratified,
	DisRegRep::Splatting::ImplementationHelper::PredefinedScratchMemory::Simple,
	DisRegRep::Container::Regionfield;

using std::tuple;
using std::ranges::for_each,
	std::bind_front, std::multiplies,
	std::views::cartesian_product, std::views::iota, std::views::transform;
using std::uniform_real_distribution;
using std::integer_sequence, std::make_integer_sequence;

DRR_SPLATTING_DEFINE_DELEGATING_FUNCTOR(Stratified) {
	this->validate(invoke_info, regionfield);
	auto& [kernel_memory, output_memory] =
		ImplementationHelper::PredefinedScratchMemory::allocateSimple<ContainerTrait>(invoke_info, regionfield, memory);

	using StratumExtentType = glm::f32vec2;
	using LengthType = StratumExtentType::length_type;
	const KernelSizeType d = this->diametre();
	const auto stratum_extent = static_cast<StratumExtentType::value_type>(d) / this->StratumCount;
	const auto stratum_bound =
		[stratum_iota = iota(KernelSizeType {}, this->StratumCount)]<LengthType... I>(
			integer_sequence<LengthType, I...>) constexpr noexcept {
			return cartesian_product(((void)I, stratum_iota)...);
		}(make_integer_sequence<LengthType, StratumExtentType::length()> {})
			| Core::View::Functional::MakeFromTuple<StratumExtentType>
			| transform(bind_front(multiplies {}, stratum_extent))
			| transform([stratum_extent](const auto coord) constexpr noexcept { return tuple(coord, coord + stratum_extent); });

	std::ranges::transform(this->convolve(Stratified::IncludeOffsetEnumeration, invoke_info, regionfield),
		output_memory.range().begin(),
		[
			&kernel_memory,
			d,
			&stratum_bound,
			secret = Stratified::generateSecret(this->Seed),
			norm_factor = stratum_bound.size()
		](auto offset_kernel) {
			kernel_memory.clear();
			for_each(stratum_bound, [&, d](const auto bound) {
				const auto [kernel_offset, kernel] = std::move(offset_kernel);
				const auto [stratum_begin, stratum_end] = bound;

				//Random state depends on both kernel and stratum offset,
				//	to ensure they get distinct states if strata from different kernels overlap.
				//Never use hash function directly on floating points due to rounding errors.
				const auto sample = [=,
					rng = Core::XXHash::RandomEngine(secret, auto(kernel_offset), DimensionType(stratum_begin))]
				<LengthType... I>(integer_sequence<LengthType, I...>) mutable {
					auto dist = tuple(uniform_real_distribution(stratum_begin[I], stratum_end[I])...);
					//Need to use clamp to round coordinates to avoid out-of-bound access due to floating point inaccuracy
					//	since max index should be one less than the size.
					//This may introduce a tiny bias towards the lower and upper bound,
					//	but should be negligible because round error is small.
					return glm::clamp(DimensionType(std::get<I>(dist)(rng)...), 0U, d - 1U);
				}(make_integer_sequence<LengthType, StratumExtentType::length()> {});
				kernel_memory.increment(kernel[sample.x][sample.y]);
			});
			return DisRegRep::Container::SplatKernel::toMask(kernel_memory, norm_factor);
		});
	return output_memory;
}

void Stratified::validate(const InvokeInfo& invoke_info, const Regionfield& regionfield) const {
	this->Base::validate(invoke_info, regionfield);

	DRR_ASSERT(this->StratumCount > 0U);
}

DRR_SPLATTING_DEFINE_SIZE_BYTE(Stratified, Simple)
DRR_SPLATTING_DEFINE_FUNCTOR_ALL(Stratified)