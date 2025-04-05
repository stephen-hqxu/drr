#include <DisRegRep/Core/Bit.hpp>

#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>

#include <algorithm>
#include <ranges>

#include <bit>

#include <limits>
#include <type_traits>

#include <cmath>
#include <cstdint>

namespace Bit = DisRegRep::Core::Bit;
using Bit::BitPerSampleResult;

using std::to_array;
using std::ranges::equal,
	std::ranges::range_value_t;
using std::numeric_limits, std::add_const_t, std::is_same_v;

namespace {

using NumberType = std::uint16_t;
using ConstNumberType = add_const_t<NumberType>;

constexpr auto Number = to_array<NumberType>({ 2, 1, 3, 2, 0, 1 });
constexpr BitPerSampleResult NumberBitPerSampleResult = Bit::minimumBitPerSample(Number);
constexpr NumberType PackedNumber = 0b10'01'11'10'00'01'01'01U;

}

TEMPLATE_TEST_CASE("BitPerSampleResult calculates storage requirements based on a given bits per sample", "[Core][Bit]", std::uint_fast8_t, std::uint_fast16_t) {
	using DataType = TestType;
	using Limit = numeric_limits<DataType>;

	GIVEN("A known bit per sample value") {
		const BitPerSampleResult::BitType bps = 1U << GENERATE(take(3U, random<std::uint_fast8_t>(0U, 3U)));

		WHEN("A bit per sample result is constructed") {
			const auto bps_result = BitPerSampleResult(BitPerSampleResult::DataTypeTag<DataType>, bps);
			const auto [bit, packing_factor, packing_factor_log2, sample_mask] = bps_result;

			THEN("All related storage requirement members are calculated correctly") {
				CHECK(bit == bps);
				CHECK(packing_factor == Limit::digits / bps);
				CHECK(packing_factor_log2 == static_cast<BitPerSampleResult::BitType>(log2(packing_factor)));

				const BitPerSampleResult::BitType mask_length = std::countr_one(sample_mask);
				CHECK(mask_length == bps);
				CHECK(std::countl_zero(sample_mask) + mask_length == numeric_limits<BitPerSampleResult::MaskType>::digits);
			}

		}

	}

}

SCENARIO("Pack an array of small unsigned integers to a single unsigned integer", "[Core][Bit]") {

	GIVEN("An array of unsigned integers") {

		WHEN("Those numbers are packed") {
			static constexpr auto Packed = Bit::pack(Number, NumberBitPerSampleResult);

			THEN("Packed number is the same type as the input array of unsigned integers") {
				STATIC_CHECK(is_same_v<decltype(Packed), ConstNumberType>);
			}

			THEN("Packed number represents the original array; essentially an array of unsigned integers with reduced bits") {
				STATIC_CHECK(Packed == PackedNumber);
			}

		}

	}

}

SCENARIO("Unpack a single unsigned integer to an array of small unsigned integers", "[Core][Bit]") {

	GIVEN("A packed unsigned integer") {

		WHEN("This number is unpacked") {
			static constexpr auto Unpacked = Bit::unpack(PackedNumber, Number.size(), NumberBitPerSampleResult);

			THEN("Unpacked range value type is the same as that of the packed unsigned integer") {
				STATIC_CHECK(is_same_v<range_value_t<decltype(Unpacked)>, NumberType>);
			}

			THEN("Unpacked numbers can be viewed as an array of unsigned integers with reduced bits") {
				STATIC_CHECK(equal(Unpacked, Number));
			}

		}

	}

}