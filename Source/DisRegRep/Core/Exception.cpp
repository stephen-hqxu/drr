#include <DisRegRep/Core/Exception.hpp>

#include <string_view>
#include <format>

#include <source_location>
#include <stdexcept>
#include <stacktrace>

using DisRegRep::Exception;

using std::string_view, std::format;
using std::source_location, std::stacktrace, std::runtime_error;

Exception::Exception(const string_view msg, const source_location& src_loc, const stacktrace& st) :
	runtime_error(
		format("Discrete Region Representation Exception\n{:-<50}\nMessage:\n{}\nIn file {} at line {}\nStacktrace: {}\n{:-<50}",
			'-', msg, src_loc.file_name(), src_loc.line(), st, '-')) { }