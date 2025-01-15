#include <DisRegRep/Splatting/Convolution/Full/VanillaOccupancy.hpp>

#include <DisRegRep-Test/Splatting/GroundTruth.hpp>

#include <catch2/catch_test_macros.hpp>

using DisRegRep::Splatting::Convolution::Full::VanillaOccupancy;

SCENARIO("Use a brute-force 2D convolution to compute splatting coefficients from a regionfield", "[Splatting][VanillaFullOccupancy]") {

	GIVEN("A vanilla full occupancy splatting") {
		const VanillaOccupancy splatting;

		WHEN("It is invoked with ground truth data") {

			THEN("Splatting coefficients computed are correct") {
				DisRegRep::Test::Splatting::GroundTruth::checkFullConvolution(splatting);
			}

		}

	}

}