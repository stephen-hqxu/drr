#include <DisRegRep/Splatting/OccupancyConvolution/Full/Fast.hpp>

#include <DisRegRep-Test/Splatting/GroundTruth.hpp>

#include <catch2/catch_test_macros.hpp>

using DisRegRep::Splatting::OccupancyConvolution::Full::Fast;

namespace GndTth = DisRegRep::Test::Splatting::GroundTruth;

SCENARIO("Use an optimised 2D convolution to compute region occupancy from a regionfield", "[Splatting][OccupancyConvolution][Full][Fast]") {

	GIVEN("A fast full occupancy convolution") {
		Fast splatting;

		THEN("Splatting coefficient matrix is transposed due to separated convolution") {
			CHECK(splatting.isTransposed());
		}

		GndTth::checkMinimumRequirement(splatting);
		GndTth::checkSplattingCoefficient(splatting);

	}

}