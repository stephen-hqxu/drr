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

	[[nodiscard]] DimensionType minimumRegionfieldDimension(const InvokeInfo&) const noexcept override;

	[[nodiscard]] DimensionType minimumOffset(const InvokeInfo&) const noexcept override;

	/**
	 * @brief Calculate the kernel diametre with the current radius.
	 *
	 * @param r The kernel radius.
	 *
	 * @return The kernel diametre, which is always a positive odd number.
	 */
	[[nodiscard]] constexpr static SizeType diametre(const RadiusType r) noexcept {
		return 2U * r + 1U;
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