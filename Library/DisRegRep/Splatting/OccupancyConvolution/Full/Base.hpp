#pragma once

#include "../Base.hpp"

#include <DisRegRep/Core/Type.hpp>

namespace DisRegRep::Splatting::Convolution::Full {

/**
 * @brief As being a full convolution, all elements covered by the kernel are taken to derive the splatting coefficients.
 */
class Base : public Convolution::Base {
public:

	/**
	 * @brief Calculate the factor to normalise from region importance to region mask by finding the kernel area, given kernel
	 * diametre.
	 *
	 * @param d The kernel diametre.
	 *
	 * @return The normalisation factor.
	 */
	[[nodiscard]] static constexpr Core::Type::RegionMask kernelNormalisationFactor(const SizeType d) noexcept {
		return d * d;
	}

};

}