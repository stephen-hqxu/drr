#include <DisRegRep/Splatting/OccupancyConvolution/Full/Vanilla.hpp>

#include <DisRegRep-Test/Splatting/GroundTruth.hpp>

#include <catch2/catch_test_macros.hpp>

using DisRegRep::Splatting::OccupancyConvolution::Full::Vanilla;

namespace GndTth = DisRegRep::Test::Splatting::GroundTruth;

SCENARIO("Use a brute-force 2D convolution to compute region occupancy from a regionfield", "[Splatting][OccupancyConvolution][Full][Vanilla]") {

	GIVEN("A vanilla full occupancy convolution") {
		Vanilla splatting;

		THEN("Splatting coefficient matrix is original") {
			CHECK_FALSE(splatting.isTransposed());
		}

		GndTth::checkMinimumRequirement(splatting);
		GndTth::checkSplattingCoefficient(splatting);

	}

}