#include <DisRegRep-Programme/Generator/Regionfield.hpp>
#include <DisRegRep-Programme/Generator/Tiff.hpp>

#include <DisRegRep-Programme/Profiler/Driver.hpp>
#include <DisRegRep-Programme/Profiler/Splatting.hpp>

#include <DisRegRep/Container/Regionfield.hpp>

#include <DisRegRep/Core/Bit.hpp>
#include <DisRegRep/Core/Exception.hpp>
#include <DisRegRep/Core/ThreadPool.hpp>

#include <DisRegRep/Image/Serialisation/Container/Regionfield.hpp>
#include <DisRegRep/Image/Tiff.hpp>

#include <DisRegRep/Info.hpp>

#include <CLI/App.hpp>
#include <CLI/Validators.hpp>

#include <yaml-cpp/yaml.h>

#include <array>
#include <unordered_map>

#include <string>
#include <string_view>
#include <tuple>

#include <algorithm>
#include <functional>

#include <chrono>
#include <format>
#include <random>
#include <utility>

#include <filesystem>
#include <iostream>
#include <print>

#include <exception>

#include <limits>

#include <cstdint>
#include <cstdlib>

namespace Generator = DisRegRep::Programme::Generator;
namespace Container = DisRegRep::Container;
namespace Image = DisRegRep::Image;
namespace Info = DisRegRep::Info;

namespace fs = std::filesystem;
using std::array, std::unordered_map,
	std::string, std::string_view, std::make_from_tuple;
using std::ranges::generate;
using std::chrono::system_clock, std::chrono::sys_seconds, std::chrono::duration_cast,
	std::format,
	std::random_device;
using std::cout, std::println;
using std::exception;
using std::numeric_limits;

namespace {

//NOLINTBEGIN(cppcoreguidelines-pro-type-member-init)
namespace Argument {

struct Profile {

	string ConfigurationFilename;
	fs::path ResultDirectory;
	DisRegRep::Core::ThreadPool::SizeType ThreadCount;

	Profile() noexcept = default;

	Profile(const Profile&) = delete;

	Profile(Profile&&) = delete;

	Profile& operator=(const Profile&) = delete;

	Profile& operator=(Profile&&) = delete;

	~Profile() = default;

	void bind(CLI::App& cmd) & {
		cmd.add_option(
			"config",
			this->ConfigurationFilename,
			"Filename is specified for the profiler configuration file."
		)	->required()
			->type_name("CONFIG")
			->check(CLI::ExistingFile);
		cmd.add_option(
			"--output,-o",
			this->ResultDirectory,
			"A directory intended for the storage of profiling results. It should be noted that a new directory is created in this directory, in which all relevant outputs are located."
		)	->required()
			->type_name("RESULT")
			->check(CLI::ExistingDirectory);
		cmd.add_option(
			"-t",
			this->ThreadCount,
			"The quantity of threads that will be utilised for the execution of each profiler job in a concurrent manner."
		)	->type_name("THREAD")
			->check(CLI::PositiveNumber)
			->default_val(1U);
	}

};

struct TiffCompression {

	enum struct Algorithm : std::uint_fast8_t {
		None,
		Lzw,
		ZStd
	};

	Algorithm Algorithm_;
	Generator::Tiff::Compression::ZStandard ZStdOption;

	constexpr TiffCompression() noexcept = default;

	TiffCompression(const TiffCompression&) = delete;

	TiffCompression(TiffCompression&&) = delete;

	TiffCompression& operator=(const TiffCompression&) = delete;

	TiffCompression& operator=(TiffCompression&&) = delete;

	constexpr ~TiffCompression() = default;

	void bind(CLI::App& cmd) & {
		using enum Algorithm;
		auto& [compression_level] = this->ZStdOption;

		cmd.add_option(
			"--compression,-c",
			this->Algorithm_,
			"Specify the compression algorithm to be employed for data being written to the TIFF image."
		)	->type_name("ALGO")
			->transform(CLI::CheckedTransformer(unordered_map<string_view, Algorithm>({
				{ "none", None },
				{ "lzw", Lzw },
				{ "zstd", ZStd }
			})))
			->default_val(None);
		cmd.add_option(
			"-l",
			compression_level,
			"Exercise control over the compression level for a specific compression algorithm. At present, it should be noted that only ZStandard is configurable."
		)	->type_name("LEVEL")
			->check(CLI::Number)
			->default_val(3);//This is the default compression level used by the official zstd command line.
	}

	void setCompression(const Image::Tiff& tif) const {
		namespace Cprs = Generator::Tiff::Compression;
		Generator::Tiff::setCompression(tif, [this] constexpr noexcept -> Cprs::Option {
			using enum Algorithm;
			switch (this->Algorithm_) {
			case None: return Cprs::None {};
			case Lzw: return Cprs::LempelZivWelch {};
			case ZStd: return this->ZStdOption;
			default: std::unreachable();
			}
		}());
	}

};

struct Regionfield {

	using SeedType = Image::Tiff::ColourPaletteRandomEngineSeed;

	enum struct Generator : std::uint_fast8_t {
		Uniform,
		Voronoi
	};

	string OutputFilename;

	Generator Generator_;
	::Generator::Regionfield::GenerateInfo GenerateInfo;
	::Generator::Regionfield::Generator::VoronoiDiagram VoronoiDiagram;

	SeedType FullSeed;

	constexpr Regionfield() noexcept = default;

	Regionfield(const Regionfield&) = delete;

	Regionfield(Regionfield&&) = delete;

	Regionfield& operator=(const Regionfield&) = delete;

	Regionfield& operator=(Regionfield&&) = delete;

	~Regionfield() = default;

	void bind(CLI::App& cmd) & {
		namespace Bit = DisRegRep::Core::Bit;
		using ::Generator::Regionfield::DimensionType;
		using DimensionArray = array<DimensionType::value_type, DimensionType::length()>;
		using enum Generator;

		auto& [resolution, region_count, seed] = this->GenerateInfo;
		auto& [centroid_count] = this->VoronoiDiagram;

		static constexpr auto SeedSequencePacking = Bit::BitPerSampleResult(
			Bit::BitPerSampleResult::DataTypeTag<SeedType>, numeric_limits<random_device::result_type>::digits);
		const auto seed_seq = [] static {
			array<SeedType, 2U> seed_seq;
			random_device seed_gen;
			generate(seed_seq, std::ref(seed_gen));
			return seed_seq;
		}();

		cmd.add_option(
			"--output,-o",
			this->OutputFilename,
			"The designation of the TIFF image is constituted by the region identifiers derived from the regionfield matrix."
		)	->required()
			->type_name("TIF");
		cmd.add_option(
			"-G",
			this->Generator_,
			"One of the built-in regionfield generators that is used for generating the regionfield matrix."
		)	->required()
			->type_name("GEN")
			->transform(CLI::CheckedTransformer(unordered_map<string_view, Generator>( {
				{ "uniform", Uniform },
				{ "voronoi", Voronoi }
			})));

		//These default settings provide a good balance between generation speed and visual diversity.
		cmd.add_option_function<DimensionArray>(
			"--dim",
			[&resolution](const DimensionArray dim) constexpr noexcept { resolution = make_from_tuple<DimensionType>(dim); },
			"The regionfield matrix is characterised by a specific dimension."
		)	->type_name("DIM")
			->delimiter('x')
			->check(CLI::PositiveNumber)
			->default_str("512x512")
			->force_callback();
		cmd.add_option(
			"--region",
			region_count,
			"Control the expected number of regions on the regionfield matrix."
		)	->type_name("COUNT")
			->check(CLI::PositiveNumber)
			->default_val(4U);
		cmd.add_option_function<SeedType>(
			"--seed",
			[&gen_seed = seed, &full_seed = this->FullSeed](const SeedType seed) constexpr noexcept {
				gen_seed = seed;
				full_seed = seed;
			},
			"Initialise the state of the random number generator employed for the generation of a regionfield."
		)	->type_name("SEED")
			->check(CLI::NonNegativeNumber)
			->run_callback_for_default()
			->default_val(Bit::pack(seed_seq, SeedSequencePacking));
		cmd.add_option(
			"--centroid",
			centroid_count,
			"Control the number of centroids or cells on a Voronoi Diagram."
		)	->type_name("COUNT")
			->check(CLI::PositiveNumber)
			->default_val(12U);
	}

	[[nodiscard]] Container::Regionfield generateRegionfield() const {
		namespace Gnrt = ::Generator::Regionfield::Generator;
		return ::Generator::Regionfield::generate(this->GenerateInfo, [this] constexpr noexcept -> Gnrt::Option {
			using enum Generator;
			switch (this->Generator_) {
			case Uniform: return Gnrt::Uniform {};
			case Voronoi: return this->VoronoiDiagram;
			default: std::unreachable();
			}
		}());
	}

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

void generateRegionfield(const Argument::Regionfield& arg_regionfield, const Argument::TiffCompression& arg_tiff_compression) {
	using RegionfieldProtocol = Image::Serialisation::Protocol<Container::Regionfield>;
	const auto& [output_filename, _1, _2, _3, full_seed] = arg_regionfield;

	RegionfieldProtocol::initialise();
	const auto regionfield_tif = Image::Tiff(output_filename, "w");
	arg_tiff_compression.setCompression(regionfield_tif);
	RegionfieldProtocol::write(regionfield_tif, arg_regionfield.generateRegionfield(), full_seed);
}

}

int main(const int argc, const char* const* const argv) try {
	auto parser = CLI::App(string(Info::Description));
	parser.footer(format("Further details can be found on the {} project homepage at {}.", Info::FullName, Info::HomePage));
	parser.set_version_flag(
		"--version,-v",
		string(Info::VersionLine)
	);
	parser.require_subcommand(1);

	Argument::Profile arg_profile;
	CLI::App& cmd_profile = *parser.add_subcommand(
		"profile",
		"Initiate the profiler and then measure the execution time of various splatting implementations."
	);
	arg_profile.bind(cmd_profile);

	Argument::TiffCompression arg_tiff_compression;
	CLI::App& group_tiff_compression = *parser.add_option_group(
		"tiff-compression",
		"Exercise control over the compression of generated data for the TIFF writer."
	)	->disabled_by_default();
	arg_tiff_compression.bind(group_tiff_compression);

	CLI::App& group_generator = *parser.add_option_group(
		"generator",
		format("The generation and processing of region data is to be carried out in a procedural manner, with the purpose of visually demonstrating the functionality of {} in the context of region splatting.", Info::FullName)
	)	->fallthrough();

	Argument::Regionfield arg_regionfield;
	CLI::App& cmd_regionfield = *group_generator.add_subcommand(
		"regionfield",
		"A built-in regionfield generator is utilised to generate a regionfield matrix, which must then be stored as a TIFF image."
	);
	CLI::TriggerOn(&cmd_regionfield, &group_tiff_compression);
	arg_regionfield.bind(cmd_regionfield);

	CLI11_PARSE(parser, argc, argv);

	if (cmd_profile) {
		runProfiler(arg_profile);
	} else if (cmd_regionfield) {
		generateRegionfield(arg_regionfield, arg_tiff_compression);
	}
	return EXIT_SUCCESS;
} catch (const exception& e) {
	DisRegRep::Core::Exception::print(e);
	return EXIT_FAILURE;
}