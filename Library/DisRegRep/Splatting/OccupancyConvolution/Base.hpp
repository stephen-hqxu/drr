#pragma once

#include "../Base.hpp"

#include <DisRegRep/Container/Regionfield.hpp>

#include <cstdint>

namespace DisRegRep::Splatting::Convolution {

/**
 * @brief A specialised way of computing region feature splatting coefficient is by utilisation of 2D convolution (because regionfield
 * in this project is defined as a 2D matrix). Implementations shall use different criterion to compute region importance, which is one
 * of the most importance coefficient.
 */
class Base : public Splatting::Base {
public:

	using KernelSizeType = std::uint_fast16_t;

	KernelSizeType Radius {}; /**< Radius of the convolution kernel. No convolution is performed if radius is zero. */

	[[nodiscard]] DimensionType minimumRegionfieldDimension(const InvokeInfo&) const noexcept override;

	[[nodiscard]] DimensionType minimumOffset() const noexcept override;

	[[nodiscard]] DimensionType maximumExtent(const DisRegRep::Container::Regionfield&, DimensionType) const noexcept override;

	/**
	 * @brief Calculate the kernel diametre with the current radius.
	 *
	 * @param r The kernel radius.
	 *
	 * @return The kernel diametre, which is always a positive odd number.
	 */
	[[nodiscard]] static constexpr KernelSizeType diametre(const KernelSizeType r) noexcept {
		return 2U * r + 1U;
	}

};

}