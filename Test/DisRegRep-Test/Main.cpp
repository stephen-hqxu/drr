#include <DisRegRep/Core/System/ProcessThreadControl.hpp>

#include <catch2/catch_session.hpp>

namespace ProcThrCtrl = DisRegRep::Core::System::ProcessThreadControl;

int main(const int argc, const char* const* const argv) {
	ProcThrCtrl::setPriority(200U);

	return Catch::Session().run(argc, argv);
}