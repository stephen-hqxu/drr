#pragma once

#include "ExecutionPolicy.hpp"

#include <DisRegRep/Container/Regionfield.hpp>
#include <DisRegRep/Core/XXHash.hpp>

#include <random>
#include <string_view>

#include <cstdint>

//Declare `DisRegRep::RegionfieldGenerator::Base::operator()`.
#define DRR_REGIONFIELD_GENERATOR_DECLARE_FUNCTOR(QUAL, THREADING) \
	void QUAL operator()( \
		const DRR_REGIONFIELD_GENERATOR_EXECUTION_POLICY_TRAIT(THREADING) ep_trait, \
		DisRegRep::Container::Regionfield& regionfield, \
		const DisRegRep::RegionfieldGenerator::Base::GenerateInfo& gen_info \
	) const
//Do `DRR_REGIONFIELD_GENERATOR_DECLARE_FUNCTOR` for every valid execution policy.
#define DRR_REGIONFIELD_GENERATOR_DECLARE_FUNCTOR_ALL(PREFIX, SUFFIX) \
	PREFIX DRR_REGIONFIELD_GENERATOR_DECLARE_FUNCTOR(, Single) SUFFIX; \
	PREFIX DRR_REGIONFIELD_GENERATOR_DECLARE_FUNCTOR(, Multi) SUFFIX
//Do `DRR_REGIONFIELD_GENERATOR_DECLARE_FUNCTOR_ALL` with the correct fixes for regionfield generator implementation.
#define DRR_REGIONFIELD_GENERATOR_DECLARE_FUNCTOR_ALL_IMPL DRR_REGIONFIELD_GENERATOR_DECLARE_FUNCTOR_ALL(, override)

//Declare a template function that delegates the call of the regionfield generator functor to here.
//This declaration should only be made private in the derived class.
#define DRR_REGIONFIELD_GENERATOR_DECLARE_DELEGATING_FUNCTOR(FUNC_QUAL, QUAL) \
	template<DisRegRep::RegionfieldGenerator::ExecutionPolicy::IsTrait EpTrait> \
	FUNC_QUAL void QUAL invokeImpl( \
		DisRegRep::Container::Regionfield& regionfield, \
		const DisRegRep::RegionfieldGenerator::Base::GenerateInfo& gen_info \
	) const
//Do `DRR_REGIONFIELD_GENERATOR_DECLARE_DELEGATING_FUNCTOR` with the correct qualifier for regionfield generator implementation.
#define DRR_REGIONFIELD_GENERATOR_DECLARE_DELEGATING_FUNCTOR_IMPL DRR_REGIONFIELD_GENERATOR_DECLARE_DELEGATING_FUNCTOR(,)

//Set the general information fields for a regionfield generator.
#define DRR_REGIONFIELD_GENERATOR_SET_INFO(NAME) \
	[[nodiscard]] constexpr std::string_view name() const noexcept override { \
		return NAME; \
	}

namespace DisRegRep::RegionfieldGenerator {

/**
 * @brief Base of all regionfield generators.
*/
class Base {
public:

	using SeedType = Core::XXHash::SeedType;

	struct GenerateInfo {

		SeedType Seed; /**< A seed used by random number generators. */

	};

protected:

	using RandomIntType = std::uint_fast16_t;
	using UniformDistributionType = std::uniform_int_distribution<RandomIntType>;

	/**
	 * @brief Generate a secret sequence using the currently set seed.
	 *
	 * @param gen_info @link GenerateInfo.
	 *
	 * @return Secret sequence with current seed.
	 */
	[[nodiscard]] static Core::XXHash::Secret generateSecret(const GenerateInfo&);

	/**
	 * @brief Create a new distribution based on the region count specified in a regionfield.
	 *
	 * @param regionfield A regionfield whose distribution is to be created.
	 * 
	 * @return Uniform integer distribution with range [0, region count).
	 */
	[[nodiscard]] static UniformDistributionType createDistribution(const Container::Regionfield&);

public:

	constexpr Base() noexcept = default;

	constexpr Base(const Base&) noexcept = default;

	constexpr Base(Base&&) noexcept = default;

	constexpr Base& operator=(const Base&) noexcept = default;

	constexpr Base& operator=(Base&&) noexcept = default;

	virtual constexpr ~Base() = default;

	/**
	 * @brief Get an identifying name for the regionfield generator.
	 *
	 * @return The generator name.
	 */
	[[nodiscard]] virtual constexpr std::string_view name() const noexcept = 0;

	/**
	 * @brief Generate regionfield.
	 *
	 * @param ep_trait Specify the execution policy trait. Multithread implementation defaults to call the singlethreaded version if
	 * not explicitly implemented by the application.
	 * @param[Out] regionfield A regionfield matrix where generated contents are stored.
	 * @param[In] gen_info @link GenerateInfo.
	 */
	DRR_REGIONFIELD_GENERATOR_DECLARE_FUNCTOR_ALL(virtual, = 0);

};

}