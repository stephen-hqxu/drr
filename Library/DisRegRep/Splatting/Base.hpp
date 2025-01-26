#pragma once

#include "Trait.hpp"

#include <DisRegRep/Container/Regionfield.hpp>
#include <DisRegRep/Container/SplattingCoefficient.hpp>

#include <any>
#include <string_view>

#include <type_traits>

#include <cstddef>

//Declare `DisRegRep::Splatting::Base::sizeByte`.
#define DRR_SPLATTING_DECLARE_SIZE_BYTE(PREFIX, QUAL, SUFFIX) \
	PREFIX DisRegRep::Splatting::Base::SizeType QUAL sizeByte(const std::any& memory) const SUFFIX
//Do `DRR_SPLATTING_DECLARE_SIZE_BYTE` for splatting implementations.
#define DRR_SPLATTING_DECLARE_SIZE_BYTE_IMPL DRR_SPLATTING_DECLARE_SIZE_BYTE([[nodiscard]],, override)

//Declare `DisRegRep::Splatting::Base::operator()`.
#define DRR_SPLATTING_DECLARE_FUNCTOR(QUAL, KERNEL, OUTPUT) \
	DRR_SPLATTING_TRAIT_CONTAINER(KERNEL, OUTPUT)::MaskOutputType& QUAL operator()( \
		const DRR_SPLATTING_TRAIT_CONTAINER(KERNEL, OUTPUT) container_trait, \
		const DisRegRep::Container::Regionfield& regionfield, \
		std::any& memory, \
		const DisRegRep::Splatting::Base::InvokeInfo& info \
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
		const DisRegRep::Container::Regionfield& regionfield, \
		std::any& memory, \
		const DisRegRep::Splatting::Base::InvokeInfo& info \
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
	using SizeType = std::size_t;

	struct InvokeInfo {

		DimensionType Offset, /**< Coordinate of the first point on the regionfield included for splatting. */
			Extent; /**< Extent covering the area on the regionfield where splatting are performed. */

	};

protected:

	/**
	 * @brief Check if given parameters are valid to be used for the selected splatting method.
	 *
	 * @param regionfield Regionfield used for splatting.
	 * @param info @link InvokeInfo.
	 */
	virtual void validate(const Container::Regionfield&, const InvokeInfo&) const;

public:

	constexpr Base() noexcept = default;

	constexpr Base(const Base&) noexcept = default;

	constexpr Base(Base&&) noexcept = default;

	constexpr Base& operator=(const Base&) noexcept = default;

	constexpr Base& operator=(Base&&) noexcept = default;

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
	 * @brief Minimum regionfield matrix dimension required by the given parameters.
	 *
	 * @param info @link InvokeInfo.
	 *
	 * @return Minimum regionfield dimension to be allocated by @link Container::Regionfield::resize.
	 */
	[[nodiscard]] virtual DimensionType minimumRegionfieldDimension(const InvokeInfo&) const noexcept;

	/**
	 * @brief Minimum regionfield matrix coordinate offset required by the given parameters.
	 *
	 * @return Minimum offset for @link InvokeInfo::Offset.
	 */
	[[nodiscard]] virtual DimensionType minimumOffset() const noexcept;

	/**
	 * @brief Query the usage of scratch memory.
	 *
	 * @param memory Scratch memory that has been used by the current splatting method for at least one computation.
	 *
	 * @return Memory usage of `memory` in bytes.
	 *
	 * @exception std::bad_any_cast If `memory` is not a valid scratch memory for this splatting.
	 */
	DRR_SPLATTING_DECLARE_SIZE_BYTE([[nodiscard]] virtual,, = 0);

	/**
	 * @brief Invoke to compute region feature splatting coefficients on a given regionfield. The splatting does not need to
	 * perform boundary checking, and the application should adjust offset to handle potential out-of-bound access.
	 *
	 * @param container_trait Specify the container trait.
	 * @param regionfield Splatting coefficients are computed for this regionfield.
	 * @param memory The scratch memory to be used in this invocation. The type is erased to allow implementation-defined behaviours.
	 * It is recommended to use the same memory instance across different invocation with the same `container_trait` to enable memory
	 * reuse. Otherwise, existing contents captured in `memory` will be destroyed if it does not contain a valid type used by the
	 * specific implementation.
	 * @param info @link InvokeInfo.
	 *
	 * @return The generated region mask for this regionfield whose memory is sourced from `memory`. It is safe to modify its contents
	 * should the application wish to.
	 */
	DRR_SPLATTING_DECLARE_FUNCTOR_ALL(virtual, = 0);

};

}