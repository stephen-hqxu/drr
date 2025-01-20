#pragma once

#include <DisRegRep/Container/SplatKernel.hpp>
#include <DisRegRep/Container/SplattingCoefficient.hpp>

#include <array>
#include <string_view>
#include <tuple>

#include <algorithm>

#include <utility>

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
 * @brief Get a representative name of the container implementation enum.
 *
 * @param impl Container implementation.
 *
 * @return String representation of `impl`.
 */
[[nodiscard]] consteval std::string_view tag(const ContainerImplementation impl) noexcept {
	using enum ContainerImplementation;
	switch (impl) {
	case Dense: return "D";
	case Sparse: return "S";
	default: std::unreachable();
	}
}

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

private:

	static constexpr auto TagCharacter = std::apply(
		[](const auto... impl) static consteval {
			using std::array, std::string_view, std::ranges::copy;

			array<string_view::value_type, sizeof...(impl)> ch {};
			auto it = ch.begin();
			((it = copy(tag(impl), it).out), ...);
			return ch;
		}, std::make_tuple(KernelContainerImplementation, OutputContainerImplementation));

public:

	static constexpr std::string_view Tag = TagCharacter; /**< Just a string representation of this container trait. */

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

/**
 * @brief All valid container trait combinations.
 */
using ContainerCombination = std::tuple<
	DRR_SPLATTING_TRAIT_CONTAINER(Dense, Dense),
	DRR_SPLATTING_TRAIT_CONTAINER(Dense, Sparse),
	DRR_SPLATTING_TRAIT_CONTAINER(Sparse, Sparse)
>;

}