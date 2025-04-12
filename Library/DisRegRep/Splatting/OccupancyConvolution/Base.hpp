#pragma once

#include "../Base.hpp"

#include <DisRegRep/Container/Regionfield.hpp>
#include <DisRegRep/Core/View/Functional.hpp>
#include <DisRegRep/Core/View/Matrix.hpp>

#include <tuple>

#include <ranges>

#include <utility>

#include <type_traits>

#include <cstdint>

namespace DisRegRep::Splatting::OccupancyConvolution {

/**
 * @brief In this category of region feature splatting, the coefficient is defined as the region occupancy within a 2D convolution
 * kernel (because regionfield in this project is defined as a 2D matrix). The region occupancy indicates the proportion of the kernel
 * area that is occupied by each region.
 */
class Base : public Splatting::Base {
public:

	using KernelSizeType = std::uint_fast32_t;

	KernelSizeType Radius {}; /**< Radius of the convolution kernel. No convolution is performed if radius is zero. */

protected:

	//Deduce if kernel offset should be enumerated when convolving a regionfield.
	static constexpr std::bool_constant<true> IncludeOffsetEnumeration;
	static constexpr std::bool_constant<false> ExcludeOffsetEnumeration;

	/**
	 * @brief Create a range that iterates through every element in the regionfield. For each element, constructs a 2D convolution
	 * kernel around the element.
	 *
	 * @tparam EnumOffset Specify if the innermost range value should enumerate kernel offset like @link std::views::enumerate.
	 *
	 * @param invoke_info @link InvokeInfo.
	 * @param regionfield Input whose region occupancies are convolved.
	 *
	 * @return A range of 2D convolution kernel.
	 */
	template<bool EnumOffset>
	[[nodiscard]] constexpr std::ranges::view auto convolve(
		std::bool_constant<EnumOffset>,
		const InvokeInfo& invoke_info,
		const DisRegRep::Container::Regionfield& regionfield
	) const noexcept {
		using std::views::cartesian_product, std::views::iota, std::views::transform,
			std::integer_sequence, std::make_integer_sequence;

		using LengthType = DimensionType::length_type;
		return [&invoke_info, r = this->Radius]<LengthType... I>(integer_sequence<LengthType, I...>) constexpr noexcept {
			const auto [offset, extent] = invoke_info;
			//It is much more clean to use std::views::take; keeping iota_view sized to allow better compiler optimisation.
			return cartesian_product([&, r] constexpr noexcept {
				const DimensionType::value_type start = offset[I] - r;
				return iota(start, start + extent[I]);
			}()...);
		}(make_integer_sequence<LengthType, DimensionType::length()> {})
			| Core::View::Functional::MakeFromTuple<DimensionType>
			| transform([
				kernel_extent = DimensionType(this->diametre()),
				rf_2d = regionfield.range2d()
			](const auto kernel_offset) constexpr noexcept {
				if constexpr (auto sliced_rf = rf_2d | Core::View::Matrix::Slice2d(kernel_offset, kernel_extent);
					EnumOffset) {
					return std::tuple(kernel_offset, std::move(sliced_rf));
				} else {
					return sliced_rf;
				}
			});
	}

public:

	[[nodiscard]] DimensionType minimumRegionfieldDimension(const InvokeInfo&) const override;

	[[nodiscard]] DimensionType minimumOffset() const override;

	[[nodiscard]] DimensionType maximumExtent(const DisRegRep::Container::Regionfield&, DimensionType) const override;

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

	/**
	 * @brief Calculate the kernel area given the kernel diametre.
	 *
	 * @param d The kernel diametre.
	 *
	 * @return Kernel area of the specified kernel.
	 */
	[[nodiscard]] static constexpr KernelSizeType area(const KernelSizeType d) noexcept {
		return d * d;
	}
	/**
	 * @brief Calculate the area of the current kernel.
	 *
	 * @return The area of the current kernel.
	 */
	[[nodiscard]] constexpr KernelSizeType area() const noexcept {
		return Base::area(this->diametre());
	}

};

}