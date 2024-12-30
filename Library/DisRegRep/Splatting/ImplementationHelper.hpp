#pragma once

#include "Base.hpp"
#include "Trait.hpp"

#include <any>
#include <memory>

#include <utility>

#include <type_traits>

//Define the function declared by `DRR_SPLATTING_DECLARE_DELEGATING_FUNCTOR`.
#define DRR_SPLATTING_DEFINE_DELEGATING_FUNCTOR(IMPL_NAME) DRR_SPLATTING_DECLARE_DELEGATING_FUNCTOR(inline, IMPL_NAME::)

//Define `DisRegRep::Splatting::Base::operator()`. No trailing comma is allowed here.
#define DRR_SPLATTING_DEFINE_FUNCTOR(IMPL_NAME, KERNEL, OUTPUT) \
	DRR_SPLATTING_DECLARE_FUNCTOR(IMPL_NAME::, KERNEL, OUTPUT) { \
		return this->invokeImpl<std::remove_const_t<decltype(container_trait)>>(info, memory); \
	}
//Do `DRR_SPLATTING_DEFINE_FUNCTOR` for every valid combination of container implementations.
#define DRR_SPLATTING_DEFINE_FUNCTOR_ALL(IMPL_NAME) \
	DRR_SPLATTING_DEFINE_FUNCTOR(IMPL_NAME, Dense, Dense) \
	DRR_SPLATTING_DEFINE_FUNCTOR(IMPL_NAME, Dense, Sparse) \
	DRR_SPLATTING_DEFINE_FUNCTOR(IMPL_NAME, Sparse, Sparse)

//Define a structure that holds scratch memory of splatting implementation.
#define DRR_SPLATTING_DEFINE_SCRATCH_MEMORY \
	template<DisRegRep::Splatting::Trait::IsContainer CtnTr> \
	class ScratchMemory
//Trait of containers that might be useful in a scratch memory.
#define DRR_SPLATTING_SCRATCH_MEMORY_CONTAINER_TRAIT using ContainerTrait = CtnTr;

/**
 * @brief Some utilities for standardising the implementation of region feature splatting.
 */
namespace DisRegRep::Splatting::ImplementationHelper {

/**
 * `ScratchMemory` is an implementation-defined scratch memory type whose allocation can be resized with argument of type `ResizeArg`.
 */
template<typename ScratchMemory, typename ResizeArg>
concept ResizableScratchMemory =
	std::is_default_constructible_v<ScratchMemory>
	&& requires(ScratchMemory scratch_memory, ResizeArg&& resize_arg) { scratch_memory.resize(std::forward<ResizeArg>(resize_arg)); };

/**
 * @brief Attempt to allocate storage for scratch memory. If `memory` does not point to a valid scratch memory as specified by
 * `ScratchMemory`, a new instance is constructed.
 *
 * @tparam ScratchMemory Type of the implementation-defined scratch memory.
 * @tparam ResizeArg Argument type to be passed to the resize function.
 *
 * @param memory Type-erased storage that holds the scratch memory.
 * @param resize_arg Resize argument.
 *
 * @return A valid scratch memory instance held by `memory`.
 */
template<typename ScratchMemory, typename ResizeArg>
requires ResizableScratchMemory<ScratchMemory, ResizeArg>
ScratchMemory& allocate(std::any& memory, ResizeArg&& resize_arg) {
	using std::any_cast,
		std::shared_ptr, std::make_shared;

	using SharedScratchMemory = shared_ptr<ScratchMemory>;
	const auto* const shared_memory = any_cast<SharedScratchMemory>(&memory);
	ScratchMemory& allocation = *(shared_memory ? *shared_memory : memory.emplace<SharedScratchMemory>(make_shared<ScratchMemory>()));
	allocation.resize(std::forward<ResizeArg>(resize_arg));
	return allocation;
}

}