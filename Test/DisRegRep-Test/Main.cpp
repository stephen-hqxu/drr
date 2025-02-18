#include <DisRegRep/Core/System/ProcessThreadControl.hpp>
#include <DisRegRep/Core/Exception.hpp>

#include <catch2/catch_session.hpp>

#include <exception>

#include <cstdlib>

namespace ProcThrCtrl = DisRegRep::Core::System::ProcessThreadControl;

using std::exception;

int main(const int argc, const char* const* const argv) try {
	ProcThrCtrl::setPriority(ProcThrCtrl::PriorityPreset::High);

	return Catch::Session().run(argc, argv);
} catch (const exception& e) {
	DisRegRep::Core::Exception::print(e);
	return EXIT_FAILURE;
}