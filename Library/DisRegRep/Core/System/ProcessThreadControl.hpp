#pragma once

#include <bitset>

#include <thread>

#include <limits>

#include <cstdint>

/**
 * @brief Platform-independent API for manipulation of process and thread.
 */
namespace DisRegRep::Core::System::ProcessThreadControl {

using Priority = std::uint8_t; /**< Control scheduling priority of a process or thread. */

/**
 * @brief Provide predefined priority settings. Should the application requires so, it is also feasible to specify an arbitrary
 * priority provided it lies within the range of [Min, Max]. The minimum and maximum priorities are two special values that guarantees
 * that the priority are set to the minimum and maximum possible value on the target platform. Any value in between is linearly
 * interpolated.
 */
namespace PriorityPreset {

using Limit = std::numeric_limits<Priority>;

inline constexpr Priority
	Min = Limit::min(),
	Max = Limit::max(),
	Low = Min + 1U,
	Medium = Max >> 1U,
	High = Max - 1U;

}

/**
 * @note It is currently unsupported on system with more than 64 threads because Windows puts threads into process groups, and it
 * requires further adjustments, which I don't want to bother with.
 */
inline constexpr std::uint_fast8_t MaxSystemThread = 64U;
/**
 * @brief LSB represents the first system thread.
 */
using AffinityMask = std::bitset<MaxSystemThread>;

/**
 * @brief Get priority of a given thread.
 *
 * @param thread Thread whose priority is to be got. Default to the calling thread if not given.
 *
 * @return Priority of `thread`.
 *
 * @exception std::system_error For any exception thrown by system API.
 */
[[nodiscard]] Priority getPriority(std::jthread* = nullptr);

/**
 * @brief Set priority of a given thread.
 *
 * @param priority New priority set to.
 * @param thread Thread whose priority is to be set. Default to the calling thread if not given.
 *
 * @exception std::system_error For any exception thrown by system API.
 */
void setPriority(Priority, std::jthread* = nullptr);

/**
 * @brief Get affinity mask of a given thread.
 *
 * @param thread Thread whose affinity mask is to be got. Default to the calling thread if not given.
 *
 * @return Affinity mask of `thread`.
 *
 * @exception std::system_error For any exception thrown by system API.
 */
[[nodiscard]] AffinityMask getAffinityMask(std::jthread* = nullptr);

/**
 * @brief Set affinity mask of a given thread.
 *
 * @param affinity_mask New affinity mask set to.
 * @param thread Thread whose affinity mask is to be set. Default to the calling thread if not given.
 *
 * @exception std::system_error For any exception thrown by system API.
 */
void setAffinityMask(AffinityMask, std::jthread* = nullptr);

}