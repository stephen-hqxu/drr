#pragma once

#include <execution>

#include <type_traits>

#include <cstdint>

//Get a fully qualified regionfield generator execution policy trait.
#define DRR_REGIONFIELD_GENERATOR_EXECUTION_POLICY_TRAIT(THREADING) \
	DisRegRep::RegionfieldGenerator::ExecutionPolicy::Trait<DisRegRep::RegionfieldGenerator::ExecutionPolicy::Threading::THREADING>

/**
 * @brief Define policies to enable the use of any parallel regionfield generation algorithm.
 */
namespace DisRegRep::RegionfieldGenerator::ExecutionPolicy {

/**
 * @brief Preferred threading behaviour. An implementation is allowed to fall back to the default threading behaviour if the selected
 * one is not applicable.
 */
enum class Threading : std::uint_fast8_t {
	Single = 0x00U,
	Multi = 0xFFU
};

/**
 * @brief Execution policy traits.
 *
 * @tparam Thr @link Threading.
 */
template<Threading Thr>
struct Trait {

	static constexpr Threading Threading_ = Thr;

	static constexpr auto Sequenced = [] static consteval noexcept {
		using namespace std::execution;
		using enum Threading;
		if constexpr (Threading_ == Multi) {
			return par;
		} else {
			return seq;
		}
	}();
	static constexpr auto Unsequenced = [] static consteval noexcept {
		using namespace std::execution;
		using enum Threading;
		if constexpr (Threading_ == Multi) {
			return par_unseq;
		} else {
			return unseq;
		}
	}();

};

//Convenience tags for specifying different execution policies when invoking a regionfield generator.
inline constexpr DRR_REGIONFIELD_GENERATOR_EXECUTION_POLICY_TRAIT(Single) SingleThreadingTrait;
inline constexpr DRR_REGIONFIELD_GENERATOR_EXECUTION_POLICY_TRAIT(Multi) MultiThreadingTrait;

/**
 * `Tr` is an execution policy trait.
 */
template<typename Tr>
concept IsTrait = std::is_same_v<Tr, Trait<Tr::Threading_>>;

}