#pragma once

#include <format>
#include <string_view>

#include <exception>
#include <source_location>
#include <stdexcept>
#include <stacktrace>

//Make an assertion on a boolean expression.
#define DRR_ASSERT(EXPRESSION) \
	do { \
		if (!(EXPRESSION)) [[unlikely]] { \
			throw DisRegRep::Core::Exception(std::format("Assertion Failure on expression:\n\t{}", #EXPRESSION)); \
		} \
	} while (false)

namespace DisRegRep::Core {

/**
 * @brief Base of all exceptions.
 */
class Exception : public std::runtime_error {
public:

	/**
	 * @brief Make a new exception object.
	 *
	 * @param msg The exception message.
	 * @param src_loc Source location.
	 * @param st Stack trace.
	 */
	Exception(
		std::string_view,
		const std::source_location& = std::source_location::current(),
		const std::stacktrace& = std::stacktrace::current()
	);

	/**
	 * @brief Print all exception messages from a potentially nested exception.
	 *
	 * @param e The exception object; may be a @link std::nested_exception.
	 */
	static void print(const std::exception&) noexcept;

};

}