#include <DisRegRep/Core/System/Error.hpp>
#include <DisRegRep/Core/System/Platform.hpp>

#ifdef DRR_CORE_SYSTEM_PLATFORM_OS_WINDOWS
#include <Windows.h>
#include <Winbase.h>
#include <Winnt.h>
#include <errhandlingapi.h>
#endif

#include <format>
#include <string>
#include <string_view>

#include <exception>
#include <system_error>

#include <cerrno>

namespace Err = DisRegRep::Core::System::Error;

using std::format, std::string, std::string_view;
using std::throw_with_nested,
	std::system_error, std::system_category, std::make_error_code, std::errc;

#ifdef DRR_CORE_SYSTEM_PLATFORM_OS_WINDOWS
void Err::throwWindows() {
	const DWORD last_error = GetLastError();
	LPSTR msg;
	const DWORD length = FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		nullptr,
		last_error,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPSTR>(&msg),
		0,
		nullptr
	);

	const string full_msg = format("Windows Error Code {} > {}", last_error, string_view(msg, length));
	LocalFree(msg);
	throw_with_nested(system_error(static_cast<int>(last_error), system_category(), full_msg));
}
#elifdef DRR_ASSERT_SYSTEM_ERROR_POSIX
void Err::throwPosix(const int ec) {
	throw_with_nested(system_error(make_error_code(errc { ec })));
}

void Err::throwPosixErrno() {
	throw_with_nested(system_error(make_error_code(errc { errno })));
}
#endif