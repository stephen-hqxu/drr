#include <DisRegRep/Splatting/Convolution/Full/FastOccupancy.hpp>

#include <DisRegRep-Test/Splatting/GroundTruth.hpp>

#include <catch2/catch_test_macros.hpp>

using DisRegRep::Splatting::Convolution::Full::FastOccupancy;

namespace GndTth = DisRegRep::Test::Splatting::GroundTruth;

SCENARIO("Use an optimised 2D convolution to compute splatting coefficients from a regionfield", "[Splatting][FastFullOccupancy]") {

	GIVEN("A fast full occupancy splatting") {
		FastOccupancy splatting;

		THEN("Splatting coefficient matrix is transposed due to separated convolution") {
			CHECK(splatting.isTransposed());
		}

		GndTth::checkMinimumRequirement(splatting);
		GndTth::checkSplattingCoefficient(splatting);

	}

}