#include <DisRegRep/Splatting/OccupancyConvolution/Sampled/Stochastic.hpp>
#include <DisRegRep/Splatting/OccupancyConvolution/Sampled/Base.hpp>
#include <DisRegRep/Splatting/ImplementationHelper.hpp>

#include <DisRegRep/Container/Regionfield.hpp>
#include <DisRegRep/Container/SplatKernel.hpp>

#include <DisRegRep/Core/View/Functional.hpp>
#include <DisRegRep/Core/Exception.hpp>
#include <DisRegRep/Core/UninitialisedAllocator.hpp>
#include <DisRegRep/Core/XXHash.hpp>

#include <vector>

#include <span>
#include <tuple>

#include <algorithm>
#include <ranges>

#include <utility>

using DisRegRep::Splatting::OccupancyConvolution::Sampled::Stochastic,
	DisRegRep::Container::Regionfield;

using std::vector, std::span, std::tuple;
using std::ranges::copy, std::ranges::transform, std::ranges::shuffle, std::ranges::for_each,
	std::views::cartesian_product, std::views::iota, std::views::take, std::views::as_const;
using std::integer_sequence, std::make_integer_sequence;

namespace {

DRR_SPLATTING_DEFINE_SCRATCH_MEMORY(ScratchMemory) {
public:

	DRR_SPLATTING_SCRATCH_MEMORY_CONTAINER_TRAIT;
	using SimpleScratchMemory = DisRegRep::Splatting::ImplementationHelper::PredefinedScratchMemory::Simple<ContainerTrait>;
	using ExtentType = typename SimpleScratchMemory::ExtentType;
	using IndexType = Regionfield::DimensionType;

	SimpleScratchMemory Simple;
	vector<IndexType, DisRegRep::Core::UninitialisedAllocator<IndexType>> Index;

	void resize(const tuple<ExtentType, Stochastic::KernelSizeType> arg) {
		const auto [extent, area] = arg;
		this->Simple.resize(extent);
		this->Index.resize(area);
	}

	[[nodiscard]] Stochastic::SizeType sizeByte() const noexcept {
		return this->Simple.sizeByte() + span(this->Index).size_bytes();
	}

};

}

DRR_SPLATTING_DEFINE_DELEGATING_FUNCTOR(Stochastic) {
	using TraitedScratchMemory = ScratchMemory<ContainerTrait>;

	this->validate(invoke_info, regionfield);
	const auto [offset, extent] = invoke_info;

	auto& [simple_memory, index_memory] = ImplementationHelper::allocate<ScratchMemory, ContainerTrait>(
		memory, tuple(typename TraitedScratchMemory::ExtentType(extent, regionfield.RegionCount), this->area()));
	auto& [kernel_memory, output_memory] = simple_memory;

	using IndexType = typename TraitedScratchMemory::IndexType;
	using LengthType = typename IndexType::length_type;
	copy(
		[radius_iota = iota(KernelSizeType {}, this->Radius)]<LengthType... I>(integer_sequence<LengthType, I...>) constexpr noexcept {
			return cartesian_product(((void)I, radius_iota)...);
		}(make_integer_sequence<LengthType, IndexType::length()> {})
			| Core::View::Functional::MakeFromTuple<IndexType>,
		index_memory.begin()
	);

	const auto index_sample = index_memory | as_const | take(this->Sample);
	transform(this->convolve(Stochastic::IncludeOffsetEnumeration, invoke_info, regionfield), output_memory.range().begin(),
		[
			&index_memory,
			&kernel_memory,
			&index_sample,
			secret = Stochastic::generateSecret(this->Seed),
			norm_factor = index_sample.size()
		](auto offset_kernel) noexcept {
			auto [offset, kernel] = std::move(offset_kernel);
			//It is more expensive to shuffle an index sequence than just taking some random samples from the kernel,
			//	but can avoid taking duplicate samples.
			shuffle(index_memory, Core::XXHash::RandomEngine(secret, auto(offset)));

			kernel_memory.clear();
			for_each(index_sample, [&kernel_memory, kernel = std::move(kernel)](const auto idx) noexcept {
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

DRR_SPLATTING_DEFINE_SIZE_BYTE(Stochastic, ScratchMemory)
DRR_SPLATTING_DEFINE_FUNCTOR_ALL(Stochastic)