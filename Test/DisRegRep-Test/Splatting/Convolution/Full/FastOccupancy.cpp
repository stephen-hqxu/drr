#include <DisRegRep/Splatting/Convolution/Full/FastOccupancy.hpp>

#include <DisRegRep-Test/Splatting/GroundTruth.hpp>

#include <catch2/catch_test_macros.hpp>

using DisRegRep::Splatting::Convolution::Full::FastOccupancy;

SCENARIO("Use an optimised 2D convolution to compute splatting coefficients from a regionfield", "[Splatting][FastFullOccupancy]") {

	GIVEN("A fast full occupancy splatting") {
		const FastOccupancy splatting;

		WHEN("It is invoked with ground truth data") {

			THEN("Splatting coefficients computed are correct") {
				DisRegRep::Test::Splatting::GroundTruth::checkFullConvolution(splatting);
			}

		}

	}

}