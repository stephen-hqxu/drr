#include <DisRegRep-Programme/Profiler/Driver.hpp>
#include <DisRegRep-Programme/Profiler/Splatting.hpp>

#include <DisRegRep/Core/Exception.hpp>
#include <DisRegRep/Core/ThreadPool.hpp>
#include <DisRegRep/Info.hpp>

#include <CLI/App.hpp>
#include <CLI/Validators.hpp>

#include <yaml-cpp/yaml.h>

#include <chrono>

#include <format>
#include <string>

#include <filesystem>
#include <iostream>
#include <print>

#include <exception>

#include <cstdint>
#include <cstdlib>

namespace Info = DisRegRep::Info;

namespace fs = std::filesystem;
using std::chrono::system_clock, std::chrono::sys_seconds, std::chrono::duration_cast;
using std::format, std::string;
using std::cout, std::println;
using std::exception;

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
			->type_name("OUT")
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
	const auto start = system_clock::now();
	println("The {} profiling engine was initiated at {:%c}.", Info::FullName, start);
	{
		const auto& [config_file, result_dir, thread_count] = arg_profile;
		const YAML::Node config = YAML::LoadFile(config_file);
		const YAML::Node& affinity_mask = config["thread affinity mask"];

		namespace Prof = DisRegRep::Programme::Profiler;
		const Prof::Splatting::ThreadPoolCreateInfo tp_info {
			.Size = thread_count,
			.AffinityMask = affinity_mask["profiler"].as<std::uint64_t>()
		};
		const auto parameter_set = config["parameter set"].as<Prof::Driver::SplattingInfo::ParameterSetType>();
		Prof::Driver::splatting({
			.ResultDirectory = &result_dir,
			.ThreadPoolCreateInfo = &tp_info,
			.BackgroundThreadAffinityMask = affinity_mask["background"].as<std::uint64_t>(),
			.Seed = config["seed"].as<Prof::Splatting::SeedType>(),
			.ProgressLog = &cout,
			.ParameterSet = &parameter_set
		});
	}
	const auto end = system_clock::now();
	println("The profiling engine exits normally at {:%c}, with a total runtime of {:%M min %S s}.", end,
		duration_cast<sys_seconds::duration>(end - start));
}

}

int main(const int argc, const char* const* const argv) try {
	auto parser = CLI::App(string(Info::Description));
	parser.footer(format("Further details can be found on the {} project homepage at {}.", Info::FullName, Info::HomePage));
	parser.set_version_flag("--version,-v", string(Info::VersionLine));
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
	DisRegRep::Core::Exception::print(e);
	return EXIT_FAILURE;
}