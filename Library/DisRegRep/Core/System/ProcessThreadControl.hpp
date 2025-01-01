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
using PriorityLimit = std::numeric_limits<Priority>;

/**
 * @brief LSB represents the first system thread.
 *
 * @note It is currently unsupported on system with more than 64 threads because Windows puts threads into process groups, and it
 * requires further adjustments, which I don't want to bother with.
 */
using AffinityMask = std::bitset<64U>;

/**
 * The minimum and maximum priorities are two special values that guarantees that the priority are set to the minimum and maximum
 * possible value on the target platform. Any value in between is linearly interpolated.
 */
inline constexpr auto MinPriority = PriorityLimit::min(),
	MaxPriority = PriorityLimit::max();

/**
 * @brief Set priority of a given thread.
 *
 * @param priority New priority set to.
 * @param thread Thread whose priority is to be set. Default to the calling thread if not given.
 */
void setPriority(Priority, std::jthread* = nullptr);

/**
 * @brief Set affinity mask of a given thread.
 *
 * @param affinity_mask New affinity mask set to.
 * @param thread Thread whose affinity mask is to be set. Default to the calling thread if not given.
 */
void setAffinityMask(AffinityMask, std::jthread* = nullptr);

}