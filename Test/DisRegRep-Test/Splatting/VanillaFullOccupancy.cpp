#include <DisRegRep/Splatting/VanillaFullOccupancy.hpp>

#include <DisRegRep-Test/Splatting/GroundTruth.hpp>

#include <catch2/catch_test_macros.hpp>

using DisRegRep::Splatting::VanillaFullOccupancy;

SCENARIO("Use a brute-force 2D convolution to compute splatting coefficients from a regionfield", "[Splatting][VanillaFullOccupancy]") {

	GIVEN("A vanilla full occupancy splatting") {
		VanillaFullOccupancy splatting;

		WHEN("It is invoked with ground truth data") {

			THEN("Splatting coefficients computed are correct") {
				DisRegRep::Test::Splatting::GroundTruth::checkFullConvolution(splatting);
			}

		}

	}

}