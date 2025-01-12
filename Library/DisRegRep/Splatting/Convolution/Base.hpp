#pragma once

#include "../Base.hpp"

#include <cstdint>

namespace DisRegRep::Splatting::Convolution {

/**
 * @brief A specialised way of computing region feature splatting coefficient is by utilisation of 2D convolution (because regionfield
 * in this project is defined as a 2D matrix). Implementations shall use different criterion to compute region importance, which is one
 * of the most importance coefficient.
 */
class Base : public Splatting::Base {
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
	[[nodiscard]] static constexpr SizeType diametre(const RadiusType r) noexcept {
		return 2U * r + 1U;
	}

};

}