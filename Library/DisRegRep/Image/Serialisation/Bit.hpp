#pragma once

#include <DisRegRep/Core/View/Arithmetic.hpp>

#include <algorithm>
#include <ranges>

#include <bit>
#include <utility>

#include <concepts>
#include <limits>
#include <type_traits>

#include <cassert>
#include <cstdint>

/**
 * @brief Common utilities of bit logic and arithmetic for serialising any data structure into an image.
 */
namespace DisRegRep::Image::Serialisation::Bit {

/**
 * @brief Computed storage requirement.
 */
struct BitPerSampleResult {

	using BitType = std::uint_fast8_t;
	using MaskType = std::uint_fast64_t;

	BitType Bit, /**< Minimum number of bits per sample for storing some data. */
		PackingFactor, /**< How many elements can be packed into an unsigned integer whose width is the same as the original data representation. */
		PackingFactorLog2; /**< Base 2 log of the packing factor. */
	MaskType SampleMask; /**< A mask to be applied to the data elements when packing them to an integer. */

	/**
	 * @brief Automatically calculate all fields given a baseline bits per sample for a particular data type.
	 *
	 * @tparam DataType Type of each element in a data array.
	 *
	 * @param bps Number of bits per sample. The behaviour is undefined unless it is a power-of-two number and no more than the maximum
	 * bit storage capacity supported by `DataType`.
	 */
	template<std::unsigned_integral DataType>
	explicit constexpr BitPerSampleResult(std::type_identity<DataType>, const BitType bps) noexcept :
		Bit(bps),
		PackingFactor(std::numeric_limits<DataType>::digits >> std::countr_zero(bps)),
		PackingFactorLog2(std::countr_zero(this->PackingFactor)),
		SampleMask((1U << bps) - 1U) {
		assert(std::has_single_bit(bps));
		assert(this->PackingFactor > 0U);
	}

};

/**
 * @brief Determine the minimum number of bits per sample can be used to store every element in a given data. This allows the
 * implementation to use a more efficient storage format for a given data when the data range is small, e.g. less than a byte per
 * element.
 *
 * @tparam Data Data type. Each element must be an unsigned integer. Signed integers will always require the number of bits equal to
 * the integer width for the sign bit in 2's complement. For floating points, consider quantisation.
 *
 * @param data Data whose minimum number of bits per sample is to be determined.
 *
 * @return @link BitPerSampleResult.
 */
template<std::ranges::input_range Data, std::unsigned_integral DataType = std::ranges::range_value_t<Data>>
[[nodiscard]] constexpr BitPerSampleResult minimumBitPerSample(Data&& data) noexcept {
	const auto data_max = std::ranges::max(std::forward<Data>(data));
	const BitPerSampleResult::BitType min_bps = std::bit_width(std::bit_floor(data_max)),
		//Computer generally does not work well with non-power-of-two bits of data; need to round it up.
		bps_power_of_two = std::bit_ceil(min_bps);
	return BitPerSampleResult(std::type_identity<DataType> {}, bps_power_of_two);
}

/**
 * @brief Pack elements in a data into a single integer of its original type, from MSB to LSB.
 *
 * @tparam Data Data type.
 *
 * @param data Data to be packed. The behaviour is undefined if there are too many data to fit into the integer, or each element is too
 * big to fit into the sample whose size is specified in `bps_result`.
 * @param bps_result @link BitPerSampleResult.
 *
 * @return An integer with all elements from `data` packed. If `data` size is less than the packing factor, padding will be applied by
 * repeating the last element of `data`.
 */
template<
	Core::View::Arithmetic::ClampToEdgePaddableRange<BitPerSampleResult::BitType> Data,
	std::unsigned_integral DataType = std::ranges::range_value_t<Data>
> requires std::ranges::viewable_range<Data>
[[nodiscard]] constexpr DataType pack(Data&& data, const BitPerSampleResult& bps_result) noexcept {
	using std::ranges::size,
		std::views::as_const, std::views::enumerate, std::views::reverse,
		std::ranges::sized_range;

	const auto [bps, packing_factor, _, sample_mask] = bps_result;
	if constexpr (sized_range<Data>) {
		assert(size(data) <= packing_factor);
	}

	DataType packed {};
	//Double reverse here is for reversing the enumeration order.
	for (const auto [shift, element] : std::forward<Data>(data)
		| as_const
		| Core::View::Arithmetic::PadClampToEdge(packing_factor)
		| reverse
		| enumerate
		| reverse//NOLINT(misc-redundant-expression)
	) [[likely]] {
		//Optimally we should mask the element to limit it within the sample.
		//Since we have warned the user, this is omitted to improve performance.
		assert((element | sample_mask) == sample_mask);
		packed |= element << shift * bps;
	}
	return packed;
}

/**
 * @brief Unpack an integer into an array of values, from MSB to LSB.
 *
 * @tparam DataType The packed integer and result type.
 *
 * @param packed A single unsigned integer that provides the data source unpacked from.
 * @param size Limit the output size so that the remaining bits are trimmed, starting from MSB.
 * @param bps_result @link BitPerSampleResult.
 *
 * @return A range of values unpacked from `packed`.
 */
template<std::unsigned_integral DataType>
[[nodiscard]] constexpr std::ranges::view auto unpack(
	const DataType packed, const std::type_identity_t<DataType> size, const BitPerSampleResult& bps_result) noexcept {
	using std::views::iota, std::views::stride, std::views::reverse, std::views::transform, std::views::take;

	const auto [bps, packing_factor, _, sample_mask] = bps_result;
	return iota(DataType {}, packing_factor)
		| stride(bps_result.Bit)
		| transform([packed, sample_mask](const auto shift) constexpr noexcept -> DataType { return packed >> shift & sample_mask; })
		| reverse
		| take(size);
}

}