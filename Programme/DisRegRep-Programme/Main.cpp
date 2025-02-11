#include <DisRegRep-Programme/Profiler/Driver.hpp>
#include <DisRegRep-Programme/Profiler/Splatting.hpp>

#include <DisRegRep/Core/ThreadPool.hpp>
#include <DisRegRep/Info.hpp>

#include <CLI/App.hpp>
#include <CLI/Validators.hpp>

#include <yaml-cpp/yaml.h>

#include <format>
#include <string>

#include <filesystem>
#include <iostream>
#include <print>

#include <exception>

#include <cstdint>
#include <cstdlib>

namespace fs = std::filesystem;
using std::format, std::string;
using std::cerr, std::println;
using std::exception, std::rethrow_if_nested;

namespace {

//NOLINTBEGIN(cppcoreguidelines-pro-type-member-init)
namespace Argument {

struct Profile {

	string ConfigurationFilename;
	fs::path ResultDirectory;
	DisRegRep::Core::ThreadPool::SizeType ThreadCount;

	Profile(CLI::App& cmd) {
		cmd.add_option("config", this->ConfigurationFilename, "Filename is specified for the profiler configuration file.")
			->required()
			->type_name("CONFIG")
			->check(CLI::ExistingFile);
		cmd.add_option("--output,-o", this->ResultDirectory, "A directory intended for the storage of profiling results. It should be noted that a new directory is created in this directory, in which all relevant outputs are located.")
			->required()
			->type_name("DIR")
			->check(CLI::ExistingDirectory);
		cmd.add_option("-t", this->ThreadCount, "The quantity of threads that will be utilised for the execution of each profiler job in a concurrent manner.")
			->type_name("THREAD")
			->check(CLI::PositiveNumber)
			->default_val(1U);
	}

	Profile(const Profile&) = delete;

	Profile(Profile&&) = delete;

	Profile& operator=(const Profile&) = delete;

	Profile& operator=(Profile&&) = delete;

	~Profile() = default;

};

}
//NOLINTEND(cppcoreguidelines-pro-type-member-init)

void runProfiler(const Argument::Profile& arg_profile) {
	const auto& [config_file, result_dir, thread_count] = arg_profile;
	const YAML::Node config = YAML::LoadFile(config_file);

	namespace Prof = DisRegRep::Programme::Profiler;
	const Prof::Splatting::ThreadPoolCreateInfo tp_info {
		.Size = thread_count,
		.AffinityMask = config["thread_affinity_mask"].as<std::uint64_t>()
	};
	const auto parameter_set = config["parameter set"].as<Prof::Driver::SplattingInfo::ParameterSetType>();
	Prof::Driver::splatting({
		.ResultDirectory = &result_dir,
		.ThreadPoolCreateInfo = &tp_info,
		.Seed = config["seed"].as<Prof::Splatting::SeedType>(),
		.ParameterSet = &parameter_set
	});
}

}

int main(const int argc, const char* const* const argv) try {
	namespace Info = DisRegRep::Info;
	auto parser = CLI::App(string(Info::Description));
	parser.footer(format("Further details can be found on the {} project homepage at {}.", Info::FullName, Info::HomePage));
	parser.set_version_flag("--version,-v", format("{} Revision {}", Info::ShortName, Info::Revision));
	parser.require_subcommand(1);

	//NOLINTBEGIN(misc-const-correctness)
	CLI::App& cmd_profile = *parser.add_subcommand("profile", "Initiate the profiler and then measure the execution time of various splatting implementations.");
	Argument::Profile arg_profile = cmd_profile;
	//NOLINTEND(misc-const-correctness)

	CLI11_PARSE(parser, argc, argv);

	if (cmd_profile) {
		runProfiler(arg_profile);
	}
	return EXIT_SUCCESS;
} catch (const exception& e) {
	//NOLINTBEGIN(misc-no-recursion)
	[](this const auto self, const exception& nested_e, const std::uint_fast8_t level) -> void {
		println(cerr, "Exception Level: {}\nMessage:\n{}\n{:#<80}", level, nested_e.what(), "#");
		try {
			rethrow_if_nested(nested_e);
		} catch (const exception& next_e) {
			self(next_e, level + 1U);
		}
	}(e, 0U);
	//NOLINTEND(misc-no-recursion)
	return EXIT_FAILURE;
}