#pragma once

#include <thread>
#include <cstdint>

namespace DisRegRep {

/**
 * @brief Control process and thread settings via OS API.
 */
namespace ProcessThreadControl {

/**
 * @brief Controls the priority of a process and thread.
 */
enum class Priority : std::uint8_t {
	Min = 0x00U,
	Low = 0x40U,
	Medium = 0x80U,
	High = 0xC0U,
	Max = 0xFFU
};

/**
 * @brief Set the priority of a given thread.
 *
 * @param thread The thread whose priority is set.
 * If not given, default to the calling thread.
 * @param priority The priority to be set to.
 *
 * @exception runtime_error If setting priority results in a failure.
 */
void setPriority(std::jthread&, Priority);
void setPriority(Priority);

}

}