#pragma once

#include "../Base.hpp"

#include <DisRegRep/Container/Regionfield.hpp>
#include <DisRegRep/Core/View/Functional.hpp>
#include <DisRegRep/Core/View/Matrix.hpp>

#include <ranges>

#include <utility>

#include <cstdint>

namespace DisRegRep::Splatting::OccupancyConvolution {

/**
 * @brief In this category of region feature splatting, the coefficient is defined as the region occupancy within a 2D convolution
 * kernel (because regionfield in this project is defined as a 2D matrix). The region occupancy indicates the proportion of the kernel
 * area that is occupied by each region.
 */
class Base : public Splatting::Base {
public:

	using KernelSizeType = std::uint_fast16_t;

	KernelSizeType Radius {}; /**< Radius of the convolution kernel. No convolution is performed if radius is zero. */

protected:

	/**
	 * @brief Create a range that iterates through every element in the regionfield. For each element, constructs a 2D convolution
	 * kernel around the element.
	 *
	 * @param invoke_info @link InvokeInfo.
	 * @param regionfield Input whose region occupancies are convolved.
	 *
	 * @return A range of 2D convolution kernel.
	 */
	[[nodiscard]] constexpr std::ranges::view auto convolve(
		const InvokeInfo& invoke_info, const DisRegRep::Container::Regionfield& regionfield) const noexcept {
		using std::views::cartesian_product, std::views::iota, std::views::transform,
			std::integer_sequence, std::make_integer_sequence;

		using LengthType = DimensionType::length_type;
		return [&invoke_info, r = this->Radius]<LengthType... I>(integer_sequence<LengthType, I...>) constexpr noexcept {
			const auto [offset, extent] = invoke_info;
			//It is much more clean to use std::views::take; keeping iota_view sized to allow better compiler optimisation.
			return cartesian_product([&, r] constexpr noexcept {
				const DimensionType::value_type start = offset[I] - r;
				return iota(start, start + extent[I]);
			}()...) | Core::View::Functional::MakeFromTuple<DimensionType>;
		}(make_integer_sequence<LengthType, DimensionType::length()> {})
			| transform([kernel_extent = DimensionType(this->diametre()), rf_2d = regionfield.range2d()](
							const auto kernel_offset) constexpr noexcept {
				  return rf_2d | Core::View::Matrix::Slice2d(kernel_offset, kernel_extent);
			  });
	}

public:

	[[nodiscard]] DimensionType minimumRegionfieldDimension(const InvokeInfo&) const noexcept override;

	[[nodiscard]] DimensionType minimumOffset() const noexcept override;

	[[nodiscard]] DimensionType maximumExtent(const DisRegRep::Container::Regionfield&, DimensionType) const noexcept override;

	/**
	 * @brief Calculate the kernel diametre given a radius.
	 *
	 * @param r The kernel radius.
	 *
	 * @return The kernel diametre, which is always a positive odd number.
	 */
	[[nodiscard]] static constexpr KernelSizeType diametre(const KernelSizeType r) noexcept {
		return 2U * r + 1U;//NOLINT(readability-math-missing-parentheses)
	}
	/**
	 * @brief Calculate the diametre of the current kernel.
	 *
	 * @return The diametre of the current kernel.
	 */
	[[nodiscard]] constexpr KernelSizeType diametre() const noexcept {
		return Base::diametre(this->Radius);
	}

};

}