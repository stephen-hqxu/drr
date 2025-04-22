#pragma once

#include "Base.hpp"

#include <DisRegRep/Container/Regionfield.hpp>

#include <span>

#include <cstdint>

namespace DisRegRep::RegionfieldGenerator {

/**
 * @brief Use the Diamond-Square algorithm to distribute region identifiers in random patterns on the regionfield matrix.
 */
class DiamondSquare final : public Base {
public:

	using DimensionType = Container::Regionfield::DimensionType;
	using SizeType = std::uint_least8_t;

	DimensionType InitialExtent = DimensionType(2U); /**< Initial extent of regionfield generated with @link Uniform. */
	/**
	 * @brief Specify the number of smoothing pass to be applied at the end of each iteration,
	 * which stochastically upscales regionfield by $2n - 1$.
	 */
	std::span<const SizeType> Iteration;

private:

	mutable Container::Regionfield PingPong;

	DRR_REGIONFIELD_GENERATOR_DECLARE_DELEGATING_FUNCTOR_IMPL;

public:

	constexpr DiamondSquare() = default;

	DiamondSquare(const DiamondSquare&) = delete;

	DiamondSquare(DiamondSquare&&) = delete;

	DiamondSquare& operator=(const DiamondSquare&) = delete;

	DiamondSquare& operator=(DiamondSquare&&) = delete;

	constexpr ~DiamondSquare() override = default;

	DRR_REGIONFIELD_GENERATOR_SET_INFO("DmSq")

	DRR_REGIONFIELD_GENERATOR_DECLARE_FUNCTOR_ALL_IMPL;

};

}