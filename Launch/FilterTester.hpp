#pragma once

#include <DisRegRep/Factory/RegionMapFactory.hpp>
#include <DisRegRep/Filter/RegionMapFilter.hpp>

#include <span>

namespace DisRegRep::Launch {

/**
 * @brief A self test to verify the correctness of region filters.
*/
namespace FilterTester {

template<size_t TestSize>
struct TestDescription {

	const RegionMapFactory* Factory;
	std::span<const RegionMapFilter* const, TestSize> Filter;

};

/**
 * @brief Perform self test.
 * 
 * @tparam TestSize The test size.
 * @param desc The test description.
 * 
 * @return The test result status.
*/
template<size_t TestSize>
bool test(const TestDescription<TestSize>&);

}

}