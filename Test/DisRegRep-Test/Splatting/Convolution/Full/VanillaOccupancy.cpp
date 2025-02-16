#include <DisRegRep/Splatting/Convolution/Full/VanillaOccupancy.hpp>

#include <DisRegRep-Test/Splatting/GroundTruth.hpp>

#include <catch2/catch_test_macros.hpp>

using DisRegRep::Splatting::Convolution::Full::VanillaOccupancy;

namespace GndTth = DisRegRep::Test::Splatting::GroundTruth;

SCENARIO("Use a brute-force 2D convolution to compute splatting coefficients from a regionfield", "[Splatting][Convolution][Full][VanillaOccupancy]") {

	GIVEN("A vanilla full occupancy splatting") {
		VanillaOccupancy splatting;

		THEN("Splatting coefficient matrix is original") {
			CHECK_FALSE(splatting.isTransposed());
		}

		GndTth::checkMinimumRequirement(splatting);
		GndTth::checkSplattingCoefficient(splatting);

	}

}