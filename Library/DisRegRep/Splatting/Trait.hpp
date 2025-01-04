#pragma once

#include <DisRegRep/Container/SplatKernel.hpp>
#include <DisRegRep/Container/SplattingCoefficient.hpp>

#include <type_traits>

#include <cstdint>

//Get a fully qualified splatting container trait.
#define DRR_SPLATTING_TRAIT_CONTAINER(KERNEL, OUTPUT) \
	DisRegRep::Splatting::Trait::Container< \
		DisRegRep::Splatting::Trait::ContainerImplementation::KERNEL, \
		DisRegRep::Splatting::Trait::ContainerImplementation::OUTPUT \
	>

/**
 * @brief Common traits of region feature splatting.
 */
namespace DisRegRep::Splatting::Trait {

/**
 * @brief Identification for the data structure used to implement the container.
 */
enum class ContainerImplementation : std::uint_fast8_t {
	Dense = 0x00U, /**< Use dense matrix to implement the container. */
	Sparse = 0xFFU /**< Use sparse matrix to implement the container. */
};

/**
 * @brief Type traits of containers used during the computation of splatting coefficients.
 *
 * @tparam Kernel Container implementation of the splatting kernel.
 * @tparam Output Container implementation of the computed coefficients.
 */
template<ContainerImplementation Kernel, ContainerImplementation Output>
struct Container {
private:

	template<ContainerImplementation Impl, typename Dense, typename Sparse>
	using SwitchContainer = std::conditional_t<Impl == ContainerImplementation::Dense, Dense, Sparse>;

public:

	static constexpr ContainerImplementation KernelContainerImplementation = Kernel,
		OutputContainerImplementation = Output;

	using KernelType = SwitchContainer<
		KernelContainerImplementation,
		DisRegRep::Container::SplatKernel::Dense,
		DisRegRep::Container::SplatKernel::Sparse
	>; /**< Container type of the splatting kernel. */
	using ImportanceOutputType = SwitchContainer<
		OutputContainerImplementation,
		DisRegRep::Container::SplattingCoefficient::DenseImportance,
		DisRegRep::Container::SplattingCoefficient::SparseImportance
	>; /**< Container type of the output that stores region importance. */
	using MaskOutputType = SwitchContainer<
		OutputContainerImplementation,
		DisRegRep::Container::SplattingCoefficient::DenseMask,
		DisRegRep::Container::SplattingCoefficient::SparseMask
	>; /**< Container type of the output that stores region mask. */

};

/**
 * `Tr` is a container trait.
 */
template<typename Tr>
concept IsContainer = std::is_same_v<Tr, Container<Tr::KernelContainerImplementation, Tr::OutputContainerImplementation>>;

}