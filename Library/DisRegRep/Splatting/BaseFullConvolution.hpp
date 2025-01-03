#pragma once

#include "Base.hpp"

#include <DisRegRep/Core/Type.hpp>

#include <cstdint>

namespace DisRegRep::Splatting {

/**
 * @brief A specialised way of computing region feature splatting coefficient is by utilisation of 2D convolution (because regionfield
 * in this project is defined as a 2D matrix). As begin a *full* convolution, all elements covered by the kernel are taken to derive
 * the result. Implementations shall use different criterion to compute region importance, which is one of the most importance
 * coefficient.
 */
class BaseFullConvolution : public Base {
public:

	using RadiusType = std::uint_fast16_t;
	using SizeType = std::uint_fast32_t;

	RadiusType Radius {}; /**< Radius of the convolution kernel. No convolution is performed if radius is zero. */

protected:

	/**
	 * @brief Check if given parameters are valid to be used for splatting methods that use full convolution.
	 */
	void validate(const InvokeInfo&) const;

	/**
	 * @brief Calculate the kernel diametre with the current radius.
	 *
	 * @note The kernel diametre is always a positive odd number.
	 *
	 * @return The kernel diametre.
	 */
	[[nodiscard]] constexpr SizeType diametre() const noexcept {
		return 2U * this->Radius + 1U;
	}

	/**
	 * @brief Calculate the factor to normalise from region importance to region mask by finding the kernel area, given kernel
	 * diametre.
	 *
	 * @param d The kernel diametre.
	 *
	 * @return The normalisation factor.
	 */
	[[nodiscard]] constexpr static Core::Type::RegionMask kernelNormalisationFactor(const SizeType d) noexcept {
		return d * d;
	}

};

}