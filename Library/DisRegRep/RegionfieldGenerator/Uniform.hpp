#pragma once

#include "Base.hpp"

namespace DisRegRep::RegionfieldGenerator {

/**
 * @brief Generate a regionfield where region identifiers are distributed uniformly random.
*/
class Uniform final : public Base {
private:

	DRR_REGIONFIELD_GENERATOR_DECLARE_DELEGATING_FUNCTOR_IMPL;

public:

	DRR_REGIONFIELD_GENERATOR_SET_INFO("Uniform")

	DRR_REGIONFIELD_GENERATOR_DECLARE_FUNCTOR_ALL_IMPL;

};

}