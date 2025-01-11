#pragma once

#include "Trait.hpp"

#include <DisRegRep/Container/Regionfield.hpp>
#include <DisRegRep/Container/SplattingCoefficient.hpp>

#include <any>
#include <string_view>

#include <type_traits>

//Declare `DisRegRep::Splatting::Base::operator()`.
#define DRR_SPLATTING_DECLARE_FUNCTOR(QUAL, KERNEL, OUTPUT) \
	DRR_SPLATTING_TRAIT_CONTAINER(KERNEL, OUTPUT)::MaskOutputType& QUAL operator()( \
		const DRR_SPLATTING_TRAIT_CONTAINER(KERNEL, OUTPUT) container_trait, \
		const DisRegRep::Splatting::Base::InvokeInfo& info, \
		std::any& memory \
	) const
//Do `DRR_SPLATTING_DECLARE_FUNCTOR` for every valid combination of container implementations.
#define DRR_SPLATTING_DECLARE_FUNCTOR_ALL(PREFIX, SUFFIX) \
	PREFIX DRR_SPLATTING_DECLARE_FUNCTOR(, Dense, Dense) SUFFIX; \
	PREFIX DRR_SPLATTING_DECLARE_FUNCTOR(, Dense, Sparse) SUFFIX; \
	PREFIX DRR_SPLATTING_DECLARE_FUNCTOR(, Sparse, Sparse) SUFFIX
//Do `DRR_SPLATTING_DECLARE_FUNCTOR_ALL` with the correct fixes for splatting implementations.
#define DRR_SPLATTING_DECLARE_FUNCTOR_ALL_IMPL DRR_SPLATTING_DECLARE_FUNCTOR_ALL(, override)

//Declare a template function that delegates the call of splatting functor to here.
//This declaration should only be made private in the derived class.
#define DRR_SPLATTING_DECLARE_DELEGATING_FUNCTOR(FUNC_QUAL, QUAL) \
	template<DisRegRep::Splatting::Trait::IsContainer ContainerTrait> \
	FUNC_QUAL ContainerTrait::MaskOutputType& QUAL invokeImpl( \
		const DisRegRep::Splatting::Base::InvokeInfo& info, \
		std::any& memory \
	) const
//Do `DRR_SPLATTING_DECLARE_DELEGATING_FUNCTOR` with the correct qualifier for splatting implementations.
#define DRR_SPLATTING_DECLARE_DELEGATING_FUNCTOR_IMPL DRR_SPLATTING_DECLARE_DELEGATING_FUNCTOR(,)

//Set the general information fields for a splatting method.
#define DRR_SPLATTING_SET_INFO(NAME, TRANSPOSED) \
	[[nodiscard]] constexpr std::string_view name() const noexcept override { \
		return NAME; \
	} \
	[[nodiscard]] constexpr bool isTransposed() const noexcept override { \
		return TRANSPOSED; \
	}

namespace DisRegRep::Splatting {

/**
 * @brief Provide a unified API for implementations of different approaches that compute the coefficient for region feature splatting,
 * given a regionfield as input.
 */
class Base {
public:

	//The reason why we take the common of these two types is because we will be using this as 2D index to access these containers.
	using DimensionType = std::common_type_t<
		Container::Regionfield::DimensionType,
		Container::SplattingCoefficient::Type::Dimension2Type
	>;

	struct InvokeInfo {

		const Container::Regionfield* Regionfield;
		DimensionType Offset, /**< Coordinate of the first point on `Regionfield` included for splatting. */
			Extent; /**< Extent covering the area on `Regionfield` to be splatted. */

	};

	constexpr Base() noexcept = default;

	Base(const Base&) = delete;

	Base(Base&&) = delete;

	Base& operator=(const Base&) = delete;

	Base& operator=(Base&&) = delete;

	virtual constexpr ~Base() = default;

	/**
	 * @brief Get a descriptive name of the splatting method.
	 *
	 * @return The splatting method name.
	 */
	[[nodiscard]] virtual constexpr std::string_view name() const noexcept = 0;

	/**
	 * @brief Determine if the resulting region feature splatting coefficient matrix is transposed of the input.
	 *
	 * @note If the return value is True, it can be done by either transposing the regionfield matrix or transposing the output to
	 * obtain the original result because matrix transpose is an invertible operation.
	 *
	 * @return True if the result is transposed.
	 */
	[[nodiscard]] virtual constexpr bool isTransposed() const noexcept = 0;

	/**
	 * @brief Invoke to compute region feature splatting coefficients on a given regionfield. The splatting does not need to
	 * perform boundary checking, and the application should adjust offset to handle potential out-of-bound access.
	 *
	 * @param container_trait Specify the container trait.
	 * @param info The invoke info.
	 * @param memory The scratch memory to be used in this invocation. The type is erased to allow implementation-defined behaviours.
	 * It is recommended to use the same memory instance across different invocation with the same `container_trait` to enable memory
	 * reuse. Otherwise, existing contents captured in `memory` will be destroyed if it does not contain a valid type used by the
	 * specific implementation.
	 *
	 * @return The generated region mask for this regionfield whose memory is sourced from `memory`. It is safe to modify its contents
	 * should the application wish to.
	 */
	DRR_SPLATTING_DECLARE_FUNCTOR_ALL(virtual, = 0);

};

}