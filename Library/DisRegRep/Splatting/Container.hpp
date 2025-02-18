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
#define DRR_SPLATTING_CONTAINER_TRAIT(KERNEL, OUTPUT) \
	DisRegRep::Splatting::Container::Trait< \
		DisRegRep::Splatting::Container::Implementation::KERNEL, \
		DisRegRep::Splatting::Container::Implementation::OUTPUT \
	>

/**
 * @brief Container usage of region feature splatting.
 */
namespace DisRegRep::Splatting::Container {

/**
 * @brief Identification for the data structure used to implement the container.
 */
enum class Implementation : std::uint_fast8_t {
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
[[nodiscard]] consteval std::string_view tag(const Implementation impl) noexcept {
	using enum Implementation;
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
template<Implementation Kernel, Implementation Output>
struct Trait {
private:

	template<Implementation Impl, typename Dense, typename Sparse>
	using SwitchContainer = std::conditional_t<Impl == Implementation::Dense, Dense, Sparse>;

public:

	static constexpr Implementation KernelImplementation = Kernel,
		OutputImplementation = Output;

private:

	static constexpr auto TagCharacter = std::apply(
		[](const auto... impl) static consteval {
			using std::array, std::string_view, std::ranges::copy;

			array<string_view::value_type, sizeof...(impl)> ch {};
			auto it = ch.begin();
			((it = copy(tag(impl), it).out), ...);
			return ch;
		}, std::tuple(KernelImplementation, OutputImplementation));

public:

	static constexpr auto Tag = std::string_view(TagCharacter); /**< Just a string representation of this container trait. */

	using KernelType = SwitchContainer<
		KernelImplementation,
		DisRegRep::Container::SplatKernel::Dense,
		DisRegRep::Container::SplatKernel::Sparse
	>; /**< Container type of the splatting kernel. */
	using ImportanceOutputType = SwitchContainer<
		OutputImplementation,
		DisRegRep::Container::SplattingCoefficient::DenseImportance,
		DisRegRep::Container::SplattingCoefficient::SparseImportance
	>; /**< Container type of the output that stores region importance. */
	using MaskOutputType = SwitchContainer<
		OutputImplementation,
		DisRegRep::Container::SplattingCoefficient::DenseMask,
		DisRegRep::Container::SplattingCoefficient::SparseMask
	>; /**< Container type of the output that stores region mask. */

};

/**
 * `Tr` is a container trait.
 */
template<typename Tr>
concept IsTrait = std::is_same_v<Tr, Trait<Tr::KernelImplementation, Tr::OutputImplementation>>;

/**
 * @brief All valid container trait combinations.
 */
using Combination = std::tuple<
	DRR_SPLATTING_CONTAINER_TRAIT(Dense, Dense),
	DRR_SPLATTING_CONTAINER_TRAIT(Dense, Sparse),
	DRR_SPLATTING_CONTAINER_TRAIT(Sparse, Sparse)
>;

}