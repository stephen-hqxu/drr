#pragma once

#include "../Base.hpp"

#include <DisRegRep/Core/Type.hpp>

namespace DisRegRep::Splatting::OccupancyConvolution::Full {

/**
 * @brief As being a full convolution, all elements covered by the kernel are taken to derive the region occupancy.
 */
class Base : public OccupancyConvolution::Base {
public:

	/**
	 * @brief Calculate the factor to normalise from region importance to region mask by finding the kernel area, given a kernel
	 * diametre.
	 *
	 * @param d The kernel diametre.
	 *
	 * @return The normalisation factor.
	 */
	[[nodiscard]] static constexpr Core::Type::RegionMask kernelNormalisationFactor(const KernelSizeType d) noexcept {
		return d * d;//NOLINT(cppcoreguidelines-narrowing-conversions, bugprone-narrowing-conversions)
	}
	/**
	 * @brief Calculate the normalisation factor of the current kernel.
	 *
	 * @return The normalisation factor of the current kernel.
	 */
	[[nodiscard]] constexpr Core::Type::RegionMask kernelNormalisationFactor() const noexcept {
		return Base::kernelNormalisationFactor(this->diametre());
	}

};

}