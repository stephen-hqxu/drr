#pragma once

#include "Base.hpp"
#include "ExecutionPolicy.hpp"

#include <type_traits>

//Define the function declared by `DRR_REGIONFIELD_GENERATOR_DECLARE_DELEGATING_FUNCTOR`.
#define DRR_REGIONFIELD_GENERATOR_DEFINE_DELEGATING_FUNCTOR(IMPL_NAME) \
	DRR_REGIONFIELD_GENERATOR_DECLARE_DELEGATING_FUNCTOR(inline, IMPL_NAME::)

//Define `DisRegRep::RegionfieldGenerator::Base::operator()`.
#define DRR_REGIONFIELD_GENERATOR_DEFINE_FUNCTOR(IMPL_NAME, THREADING) \
	DRR_REGIONFIELD_GENERATOR_DECLARE_FUNCTOR(IMPL_NAME::, THREADING) { \
		return this->invokeImpl<std::remove_const_t<decltype(ep_trait)>>(regionfield, gen_info); \
	}
//Do `DRR_REGIONFIELD_GENERATOR_DEFINE_FUNCTOR` for every valid execution policy.
#define DRR_REGIONFIELD_GENERATOR_DEFINE_FUNCTOR_ALL(IMPL_NAME) \
	DRR_REGIONFIELD_GENERATOR_DEFINE_FUNCTOR(IMPL_NAME, Single) \
	DRR_REGIONFIELD_GENERATOR_DEFINE_FUNCTOR(IMPL_NAME, Multi)