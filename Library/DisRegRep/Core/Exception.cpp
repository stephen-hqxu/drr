#include <DisRegRep/Core/Exception.hpp>
#include <DisRegRep/Info.hpp>

#include <string_view>
#include <format>

#include <iostream>
#include <print>

#include <exception>
#include <source_location>
#include <stdexcept>
#include <stacktrace>

#include <cstdint>

using DisRegRep::Core::Exception;

using std::string_view, std::format;
using std::cerr, std::println;
using std::exception, std::rethrow_if_nested,
	std::source_location, std::stacktrace, std::runtime_error;

Exception::Exception(const string_view msg, const source_location& src_loc, const stacktrace& st) :
	runtime_error(format("[{} Exception]\n> {}::({}, {}) <\n{}\nStack Trace:\n{}",
		Info::FullName,
		src_loc.file_name(), src_loc.line(), src_loc.column(),
		msg,
		st
	)) { }

void Exception::print(const exception& e) noexcept {
	//NOLINTNEXTLINE(misc-no-recursion, bugprone-exception-escape)
	[](this const auto self, const exception& current_e, const std::uint_fast8_t level) noexcept -> void {
		println(cerr, "{:*<30}\n*** {: ^22} ***\n{:*<30}\n{}",
			'*', format("Exception Level {}", level), '*',
			current_e.what()
		);
		try {
			rethrow_if_nested(current_e);
		} catch (const exception& next_e) {
			self(next_e, level + 1U);
		}
	}(e, 0U);
}