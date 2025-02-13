#include <DisRegRep/Core/System/ProcessThreadControl.hpp>

#include <catch2/catch_session.hpp>

#include <exception>
#include <iostream>
#include <print>

#include <cstdlib>

namespace ProcThrCtrl = DisRegRep::Core::System::ProcessThreadControl;

using std::exception, std::rethrow_if_nested,
	std::cerr, std::println;

int main(const int argc, const char* const* const argv) try {
	ProcThrCtrl::setPriority(ProcThrCtrl::PriorityPreset::High);

	return Catch::Session().run(argc, argv);
} catch (const exception& e) {
	println("Uncaught exception during test run with the following message:");
	//NOLINTBEGIN(misc-no-recursion)
	[](this const auto self, const exception& current_e) -> void {
		println(cerr, "{}\n{:#<80}", current_e.what(), "#");
		try {
			rethrow_if_nested(current_e);
		} catch (const exception& next_e) {
			self(next_e);
		}
	}(e);
	//NOLINTEND(misc-no-recursion)
	return EXIT_FAILURE;
}