#pragma once

#include "../Format.hpp"

#include <array>
#include <any>

#include <algorithm>
#include <type_traits>
#include <concepts>

namespace DisRegRep {

namespace _Detail {

//Specify an index type.
template<typename T>
concept Indexable = std::signed_integral<T> || std::unsigned_integral<T>;

}

/**
 * @brief A fundamental class for different implementation of region map filters.
*/
class RegionMapFilter {
public:

	/**
	 * @brief The configuration for filter launch.
	*/
	struct LaunchDescription {

		const Format::RegionMap* Map;/**< The region map. */
		std::array<size_t, 2u> Offset,/**< Start offset. */
			Extent;/**< The extent of covered area to be filtered. */
		size_t Radius;/**< Radius of filter. */

	};

protected:

	/**
	 * @brief A helper class to facilitate indexing.
	*/
	class Indexer {
	private:

		const RegionMapFilter::LaunchDescription& Description;
		//Row length of the working area.
		const size_t WorkingRowLength;

	public:

		using SignedSize_t = std::make_signed_t<size_t>;

		constexpr Indexer(const RegionMapFilter::LaunchDescription& desc) noexcept :
			Description(desc), WorkingRowLength(this->Description.Extent[0] * this->Description.Map->RegionCount) {

		}

		constexpr ~Indexer() = default;

		/**
		 * @brief Convert a number to signed.
		 * 
		 * @tparam T Type of integer.
		 * @tparam S Array length
		 * @param i The integer to be converted.
		 * @param nums An array of integers to be converted.
		 * 
		 * @return The signed number.
		*/
		template<_Detail::Indexable T>
		constexpr static auto toSigned(const T i) noexcept {
			return static_cast<std::make_signed_t<T>>(i);
		}
		template<_Detail::Indexable T, size_t S>
		constexpr static auto toSigned(const std::array<T, S>& nums) noexcept {
			std::array<std::make_signed_t<T>, S> snums;
			std::ranges::transform(nums, snums.begin(), [](const auto i) constexpr noexcept { return Indexer::toSigned(i); });
			return snums;
		}

		/**
		 * @brief Get index into region map.
		 * Offset has been added automatically.
		 * 
		 * @tparam T1, T2 The type of coordinate. Might be signed.
		 * @param x X coordinate.
		 * @param y Y coordinate.
		 * 
		 * @return Region map index, which must be positive.
		*/
		template<_Detail::Indexable T1, _Detail::Indexable T2 = std::type_identity_t<T1>>
		constexpr size_t region(const T1 x, const T2 y) const noexcept {
			const auto [off_x, off_y] = this->Description.Offset;
			return off_x + x + (off_y + y) * this->Description.Map->Dimension[0];
		}

		/**
		 * @brief Get index into single histogram.
		 * 
		 * @tparam T1, T2, T3 The type of coordinate. Might be signed.
		 * @param x X coordinate.
		 * @param y Y coordinate.
		 * @param bin Bin index.
		 * 
		 * @return Histogram index to the first bin.
		*/
		template<_Detail::Indexable T1, _Detail::Indexable T2 = std::type_identity_t<T1>,
			_Detail::Indexable T3 = std::type_identity_t<T1>>
		constexpr size_t histogram(const T1 x, const T2 y, const T3 bin) const noexcept {
			return bin + x * this->Description.Map->RegionCount + y * this->WorkingRowLength;
		}

	};

public:

	constexpr RegionMapFilter() noexcept = default;

	constexpr virtual ~RegionMapFilter() = default;

	/**
	 * @brief Allocate histogram memory for launch.
	 * 
	 * @param desc The filter launch description.
	 * 
	 * @return Allocated memory.
	*/
	virtual std::any allocateHistogram(const RegionMapFilter::LaunchDescription& desc) const = 0;

	/**
	 * @brief Perform filter on region map.
	 * This filter does no boundary checking, application should adjust offset to handle padding.
	 * 
	 * @param desc The filter launch description.
	 * @param memory The memory allocated for this launch.
	 * The behaviour is undefined if this memory is not allocated with compatible launch description.
	 * The compatibility depends on implementation of derived class.
	 * 
	 * @return The generated normalised single histogram for this region map.
	 * The memory is held by the `memory` input.
	*/
	virtual const Format::DenseNormSingleHistogram& filter(const RegionMapFilter::LaunchDescription& desc,
		std::any& memory) const = 0;

};

}