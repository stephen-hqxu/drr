#pragma once

#include "Base.hpp"
#include "Container.hpp"

#include <DisRegRep/Container/Regionfield.hpp>

#include <any>
#include <tuple>
#include <variant>

#include <memory>

#include <utility>

#include <concepts>
#include <type_traits>

//Define `DisRegRep::Splatting::Base::sizeByte`.
#define DRR_SPLATTING_DEFINE_SIZE_BYTE(IMPL_NAME, MEM_NAME) \
	DRR_SPLATTING_DECLARE_SIZE_BYTE(, IMPL_NAME::, ) { \
		return DisRegRep::Splatting::ImplementationHelper::sizeByte<MEM_NAME>(memory); \
	}

//Define the function declared by `DRR_SPLATTING_DECLARE_DELEGATING_FUNCTOR`.
#define DRR_SPLATTING_DEFINE_DELEGATING_FUNCTOR(IMPL_NAME) DRR_SPLATTING_DECLARE_DELEGATING_FUNCTOR(inline, IMPL_NAME::)

//Define `DisRegRep::Splatting::Base::operator()`. No trailing comma is allowed here.
#define DRR_SPLATTING_DEFINE_FUNCTOR(IMPL_NAME, KERNEL, OUTPUT) \
	DRR_SPLATTING_DECLARE_FUNCTOR(IMPL_NAME::, KERNEL, OUTPUT) { \
		return this->invokeImpl<std::remove_const_t<decltype(container_trait)>>(invoke_info, regionfield, memory); \
	}
//Do `DRR_SPLATTING_DEFINE_FUNCTOR` for every valid combination of container implementations.
#define DRR_SPLATTING_DEFINE_FUNCTOR_ALL(IMPL_NAME) \
	DRR_SPLATTING_DEFINE_FUNCTOR(IMPL_NAME, Dense, Dense) \
	DRR_SPLATTING_DEFINE_FUNCTOR(IMPL_NAME, Dense, Sparse) \
	DRR_SPLATTING_DEFINE_FUNCTOR(IMPL_NAME, Sparse, Sparse)

//Define a structure that holds scratch memory of splatting implementation.
#define DRR_SPLATTING_DEFINE_SCRATCH_MEMORY(MEM_NAME) \
	template<DisRegRep::Splatting::Container::IsTrait CtnTr> \
	class MEM_NAME
//Trait of containers that might be useful in a scratch memory.
#define DRR_SPLATTING_SCRATCH_MEMORY_CONTAINER_TRAIT using ContainerTrait = CtnTr;

/**
 * @brief Some utilities for standardising the implementation of region feature splatting.
 */
namespace DisRegRep::Splatting::ImplementationHelper {

/**
 * All possible instantiations of `ScratchMemory` given container trait combinations.
 */
template<template<Container::IsTrait> typename ScratchMemory>
using ScratchMemoryCombination = decltype(std::apply(
	[]<typename... Trait>(Trait...) -> std::default_initializable auto { return std::tuple<ScratchMemory<Trait>...> {}; },
	Container::Combination));

/**
 * Internal, type-erased scratch memory type, which is basically a variant of every @link ScratchMemoryCombination.
 */
template<template<Container::IsTrait> typename ScratchMemory>
using ScratchMemoryInternal =
	decltype(std::apply([]<typename... Mem>(const Mem&...) -> std::default_initializable auto { return std::variant<Mem...> {}; },
		ScratchMemoryCombination<ScratchMemory> {}));

/**
 * `ScratchMemory` is an implementation-defined scratch memory type whose allocation can be resized with argument of type `ResizeArg`.
 */
template<typename ScratchMemory, typename ResizeArg>
concept ResizableScratchMemory =
	std::is_default_constructible_v<ScratchMemory>
	&& requires(ScratchMemory scratch_memory, ResizeArg&& resize_arg) { scratch_memory.resize(std::forward<ResizeArg>(resize_arg)); };

/**
 * `ScratchMemory` is an implementation-defined scratch memory type whose memory usage can be queried.
 */
template<typename ScratchMemory>
concept SizedScratchmemory = requires(const ScratchMemory scratch_memory) {
	{ scratch_memory.sizeByte() } -> std::unsigned_integral;
};

/**
 * @brief Attempt to allocate storage for scratch memory. If `memory` does not point to a valid scratch memory as specified by
 * `ScratchMemory`, a new instance is constructed.
 *
 * @tparam ScratchMemory Type of the implementation-defined scratch memory.
 * @tparam Trait Specify the container trait that `ScratchMemory` uses.
 * @tparam ResizeArg Argument type to be passed to the resize function.
 *
 * @param memory Type-erased storage that holds the scratch memory.
 * @param resize_arg Resize argument.
 *
 * @return A valid scratch memory instance held by `memory`.
 */
template<
	template<Container::IsTrait> typename ScratchMemory,
	Container::IsTrait Trait,
	typename ResizeArg,
	ResizableScratchMemory<ResizeArg> TraitedScratchMemory = ScratchMemory<Trait>
>
[[nodiscard]] TraitedScratchMemory& allocate(std::any& memory, ResizeArg&& resize_arg) {
	using std::any_cast, std::get_if,
		std::shared_ptr, std::make_shared;

	using Internal = ScratchMemoryInternal<ScratchMemory>;
	using SharedScratchMemory = shared_ptr<Internal>;

	const auto* const shared_memory = any_cast<SharedScratchMemory>(&memory);
	Internal& internal = *(shared_memory ? *shared_memory : memory.emplace<SharedScratchMemory>(make_shared<Internal>()));

	auto* const maybe_allocation = get_if<TraitedScratchMemory>(&internal);
	TraitedScratchMemory& allocation = maybe_allocation ? *maybe_allocation : internal.template emplace<TraitedScratchMemory>();

	allocation.resize(std::forward<ResizeArg>(resize_arg));
	return allocation;
}

/**
 * @brief Get memory usage.
 * 
 * @tparam ScratchMemory Type of the implementation-defined scratch memory.
 * 
 * @param memory Type-erased storage that holds the scratch memory.
 * 
 * @return Memory usage in bytes.
 */
template<template<Container::IsTrait> typename ScratchMemory>
requires(std::apply(
	[]<typename... Mem>(const Mem&...) { return (SizedScratchmemory<Mem> && ...); }, ScratchMemoryCombination<ScratchMemory> {}))
[[nodiscard]] Base::SizeType sizeByte(const std::any& memory) {
	using std::any_cast, std::visit, std::shared_ptr;
	return visit([](const auto& allocation) static noexcept { return allocation.sizeByte(); },
		*any_cast<const shared_ptr<ScratchMemoryInternal<ScratchMemory>>&>(memory));
}

/**
 * @brief Some commonly used scratch memory structures that may shared by different splatting implementations.
 */
namespace PredefinedScratchMemory {

/**
 * @brief A simple scratch memory consists of a kernel to cache intermediate region importance, and a container for storing region
 * mask transferred from the kernel.
 */
DRR_SPLATTING_DEFINE_SCRATCH_MEMORY(Simple) {
public:

	DRR_SPLATTING_SCRATCH_MEMORY_CONTAINER_TRAIT;

	using ExtentType = typename ContainerTrait::MaskOutputType::Dimension3Type;

	typename ContainerTrait::KernelType Kernel;
	typename ContainerTrait::MaskOutputType Output;

	/**
	 * @brief Allocate scratch memory.
	 *
	 * @param extent Specify width, height and number of region for the containers.
	 */
	void resize(const ExtentType extent) {
		this->Kernel.resize(extent.z);
		this->Output.resize(extent);
	}

	/**
	 * @brief Get scratch memory size in bytes.
	 *
	 * @return Number of byte allocated to the scratch memory.
	 */
	[[nodiscard]] Base::SizeType sizeByte() const noexcept {
		using std::apply, std::tie;
		return apply([](const auto&... member) static noexcept { return (member.sizeByte() + ...); }, tie(this->Kernel, this->Output));
	}

};

/**
 * @brief Allocate storage for @link Simple scratch memory.
 *
 * @tparam Trait Specify the container trait that @link Simple uses.
 *
 * @param invoke_info @link Base::InvokeInfo.
 * @param regionfield Regionfield to be splatted.
 * @param memory Type-erased storage that holds the scratch memory.
 *
 * @return A valid @link Simple scratch memory instance held by `memory`.
 */
template<Container::IsTrait Trait>
[[nodiscard]] Simple<Trait>& allocateSimple(
	const Base::InvokeInfo& invoke_info, const DisRegRep::Container::Regionfield& regionfield, std::any& memory) {
	return allocate<Simple, Trait>(memory, typename Simple<Trait>::ExtentType(invoke_info.Extent, regionfield.RegionCount));
}

}

}