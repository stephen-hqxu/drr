#include <DisRegRep-Programme/Generator/Regionfield.hpp>
#include <DisRegRep-Programme/Generator/Tiff.hpp>

#include <DisRegRep-Programme/Profiler/Driver.hpp>
#include <DisRegRep-Programme/Profiler/Splatting.hpp>

#include <DisRegRep/Container/Regionfield.hpp>
#include <DisRegRep/Container/SplattingCoefficient.hpp>

#include <DisRegRep/Core/Bit.hpp>
#include <DisRegRep/Core/Exception.hpp>
#include <DisRegRep/Core/ThreadPool.hpp>

#include <DisRegRep/Image/Serialisation/Container/Regionfield.hpp>
#include <DisRegRep/Image/Serialisation/Container/SplattingCoefficient.hpp>
#include <DisRegRep/Image/Tiff.hpp>

#include <DisRegRep/Info.hpp>

#include <CLI/CLI.hpp>
#include <yaml-cpp/yaml.h>

#include <array>
#include <unordered_map>

#include <string>
#include <string_view>
#include <tuple>
#include <variant>

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

#include <concepts>
#include <limits>
#include <type_traits>

#include <cstdint>
#include <cstdlib>

namespace Generator = DisRegRep::Programme::Generator;
namespace Container = DisRegRep::Container;
namespace Image = DisRegRep::Image;
namespace Info = DisRegRep::Info;
using RegionfieldProtocol = Image::Serialisation::Protocol<Container::Regionfield>;
using DenseMaskProtocol = Image::Serialisation::Protocol<Container::SplattingCoefficient::DenseMask>;

namespace fs = std::filesystem;
using std::array, std::unordered_map,
	std::string, std::string_view,
	std::make_from_tuple, std::variant_alternative_t;
using std::ranges::generate;
using std::chrono::system_clock, std::chrono::sys_seconds, std::chrono::duration_cast,
	std::format,
	std::random_device;
using std::cout, std::println;
using std::exception;
using std::unsigned_integral, std::numeric_limits, std::common_type_t;

namespace {

template<typename Vec>
using VectorArray = array<typename Vec::value_type, Vec::length()>;

template<unsigned_integral Seed>
[[nodiscard]] Seed defaultSeed() {
	namespace Bit = DisRegRep::Core::Bit;
	static constexpr auto SeedSequencePacking =
		Bit::BitPerSampleResult(Bit::BitPerSampleResult::DataTypeTag<Seed>, numeric_limits<random_device::result_type>::digits);

	array<Seed, SeedSequencePacking.PackingFactor> seed_seq;
	random_device seed_gen;
	generate(seed_seq, std::ref(seed_gen));
	return Bit::pack(seed_seq, SeedSequencePacking);
}

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
			"config-yaml",
			this->ConfigurationFilename,
			"Filename is specified for the profiler configuration YAML file."
		)	->required()
			->type_name("YAML")
			->check(CLI::ExistingFile);
		cmd.add_option(
			"result",
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

	enum struct Compression : std::uint_fast8_t {
		None,
		Lzw,
		ZStd
	} Compression_ = Compression::None;
	Generator::Tiff::Compression::ZStandard ZStandard {
		//This is the default compression level used by the official zstd command line.
		.CompressionLevel = 3
	};

	constexpr TiffCompression() noexcept = default;

	TiffCompression(const TiffCompression&) = delete;

	TiffCompression(TiffCompression&&) = delete;

	TiffCompression& operator=(const TiffCompression&) = delete;

	TiffCompression& operator=(TiffCompression&&) = delete;

	constexpr ~TiffCompression() = default;

	void bind(CLI::App& cmd) & {
		using enum Compression;
		auto& [zstd_compression_level] = this->ZStandard;

		using ZStdCompressionLevel = decltype(zstd_compression_level);

		cmd.add_flag_callback(
			"--lzw",
			[this] constexpr noexcept { this->Compression_ = Lzw; },
			"Use the Lempel-Ziv & Welch compression algorithm."
		);
		cmd.add_option_function<ZStdCompressionLevel>(
			"--zstd",
			[this, &zstd_compression_level](const ZStdCompressionLevel level) constexpr noexcept {
				this->Compression_ = ZStd;
				zstd_compression_level = level;
			},
			"Use the Z-Standard compression algorithm, with the option of exercising control over its compression level."
		)
			->expected(0, 1)
			->type_name("LEVEL")
			->check(CLI::Number)
			->default_val(zstd_compression_level);
	}

	void setCompression(const Image::Tiff& tif) const {
		namespace Cprs = Generator::Tiff::Compression;
		Generator::Tiff::setCompression(tif, [this] noexcept -> Cprs::Option {
			using enum Compression;
			switch (this->Compression_) {
			case None: return Cprs::None {};
			case Lzw: return Cprs::LempelZivWelch {};
			case ZStd: return this->ZStandard;
			default: std::unreachable();
			}
		}());
	}

};

struct Regionfield {

	string OutputFilename;

	enum struct Generator : std::uint_fast8_t {
		Uniform,
		Voronoi
	} Generator_;
	::Generator::Regionfield::GenerateInfo GenerateInfo;
	::Generator::Regionfield::Generator::VoronoiDiagram VoronoiDiagram;

	constexpr Regionfield() noexcept = default;

	Regionfield(const Regionfield&) = delete;

	Regionfield(Regionfield&&) = delete;

	Regionfield& operator=(const Regionfield&) = delete;

	Regionfield& operator=(Regionfield&&) = delete;

	constexpr ~Regionfield() = default;

	void bind(CLI::App& cmd) & {
		using enum Generator;

		auto& [resolution, region_count, rf_gen_info] = this->GenerateInfo;
		auto& [seed] = rf_gen_info;
		auto& [centroid_count] = this->VoronoiDiagram;

		using ResolutionType = decltype(resolution);
		using ResolutionArray = VectorArray<ResolutionType>;
		using SeedType = decltype(seed);

		cmd.add_option(
			"regionfield-tif",
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
			->transform(CLI::CheckedTransformer(unordered_map<string_view, Generator> {
				{ "uniform", Uniform },
				{ "voronoi", Voronoi }
			}));

		//These default settings provide a good balance between generation speed and visual diversity.
		cmd.add_option_function<ResolutionArray>(
			"--dim",
			[&resolution](const ResolutionArray res) constexpr noexcept { resolution = make_from_tuple<ResolutionType>(res); },
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
		cmd.add_option(
			"--seed",
			seed,
			"Initialise the state of the random number generator employed for the generation of a regionfield."
		)	->type_name("SEED")
			->check(CLI::NonNegativeNumber)
			->default_val(defaultSeed<SeedType>());
		cmd.add_option(
			"--centroid",
			centroid_count,
			"Control the number of centroids or cells on a Voronoi Diagram."
		)	->type_name("COUNT")
			->check(CLI::PositiveNumber)
			->default_val(12U);
	}

	[[nodiscard]] constexpr auto seed() const noexcept {
		return this->GenerateInfo.RegionfieldGeneratorGenerateInfo.Seed;
	}

	[[nodiscard]] Container::Regionfield generateRegionfield() const {
		namespace Gnrt = ::Generator::Regionfield::Generator;
		return ::Generator::Regionfield::generate(this->GenerateInfo, [this] noexcept -> Gnrt::Option {
			using enum Generator;
			switch (this->Generator_) {
			case Uniform: return Gnrt::Uniform {};
			case Voronoi: return this->VoronoiDiagram;
			default: std::unreachable();
			}
		}());
	}

};

struct Splat {

	string RegionfieldFilename, OutputFilename;

	enum struct Splatting : std::uint_fast8_t {
		Full,
		Stochastic = 10U,
		Stratified,
		Systematic
	} Splatting_;
	Generator::Regionfield::Splatting::OccupancyConvolution::SplatInfo OCSplatInfo;
	Generator::Regionfield::Splatting::OccupancyConvolution::Sampled::Stochastic StochasticSampled;
	Generator::Regionfield::Splatting::OccupancyConvolution::Sampled::Stratified StratifiedSampled;
	Generator::Regionfield::Splatting::OccupancyConvolution::Sampled::Systematic SystematicSampled;

	constexpr Splat() noexcept = default;

	Splat(const Splat&) = delete;

	Splat(Splat&&) = delete;

	Splat& operator=(const Splat&) = delete;

	Splat& operator=(Splat&&) = delete;

	constexpr ~Splat() = default;

	void bind(CLI::App& cmd) & {
		using enum Splatting;
		auto& [radius] = this->OCSplatInfo;
		auto& [sample, seed_stochastic] = this->StochasticSampled;
		auto& [stratum_count, seed_stratified] = this->StratifiedSampled;
		auto& [first_sample, interval] = this->SystematicSampled;

		using SeedType = common_type_t<
			decltype(seed_stochastic),
			decltype(seed_stratified)
		>;
		using ExtentType = common_type_t<
			decltype(first_sample),
			decltype(interval)
		>;
		using ExtentArray = VectorArray<ExtentType>;

		cmd.add_option(
			"regionfield-tif",
			this->RegionfieldFilename,
			"A TIFF image that supplies region identifiers. The region feature splatting mask for the regionfield matrix is computed."
		)
			->required()
			->type_name("TIF");
		cmd.add_option(
			"mask-tif",
			this->OutputFilename,
			"The region feature splatting mask, which has been computed from the input regionfield matrix, is saved to the specified TIFF image."
		)
			->required()
			->type_name("TIF");

		cmd.add_option(
			"-S",
			this->Splatting_,
			"Select an algorithm for the calculation of the region feature splatting coefficient."
		)
			->required()
			->type_name("SPLAT")
			->transform(CLI::CheckedTransformer(unordered_map<string_view, Splatting> {
				{ "full", Full },
				{ "stochastic", Stochastic },
				{ "stratified", Stratified },
				{ "systematic", Systematic }
			}));
		//It is not easy to pick a default radius, since it depends on the dimension of the regionfield matrix.
		//It is an error if the convolution kernel is too large and goes over the matrix boundary.
		cmd.add_option(
			"--radius,-r",
			radius,
			"In the context of convolution-based region feature splatting, it is imperative to specify the radius of the convolution kernel."
		)
			->required()
			->type_name("RADIUS")
			->check(CLI::NonNegativeNumber);
		cmd.add_option_function<SeedType>(
			   "--seed",
			   [&seed_stochastic, &seed_stratified](const SeedType seed) constexpr noexcept {
				   seed_stochastic = seed;
				   seed_stratified = seed;
			   },
			   "The initial state of the random sampler employed in certain sampled convolution-based splatting processes may be seeded."
		)
			->type_name("SEED")
			->check(CLI::NonNegativeNumber)
			->default_val(defaultSeed<SeedType>());
		//The number **5** is a magic number here,
		//	which is the typical kernel diametre where the fast convolution starts to out-performs the vanilla convolution.
		cmd.add_option(
			"--sample",
			sample,
			"In the context of stochastic sampling, specify the number of random elements to be extracted from the convolution kernel in order to compute the region splatting coefficient."
		)
			->type_name("SAMPLE")
			->check(CLI::PositiveNumber)
			->default_val(25U);
		cmd.add_option(
			"--stratum",
			stratum_count,
			"In the context of stratified sampling, specify the number of strata that the convolution kernel will be divided into along each axis."
		)
			->type_name("COUNT")
			->check(CLI::PositiveNumber)
			->default_val(5U);
		cmd.add_option_function<ExtentArray>(
			"--first",
			[&first_sample](const ExtentArray first) constexpr noexcept { first_sample = make_from_tuple<ExtentType>(first); },
			"In the context of systematic sampling, the coordinate of the first element on the convolution kernel must be specified."
		)
			->type_name("COORD")
			->delimiter(',')
			->check(CLI::NonNegativeNumber)
			->default_str("0,0")
			->force_callback();
		cmd.add_option_function<ExtentArray>(
			"--interval",
			[&interval](const ExtentArray spacing) constexpr noexcept { interval = make_from_tuple<ExtentType>(spacing); },
			"In the context of systematic sampling, the number of elements to skip before taking the next one."
		)
			->type_name("SKIP")
			->delimiter(',')
			->check(CLI::PositiveNumber)
			->default_str("5,5")
			->force_callback();
	}

	[[nodiscard]] Container::SplattingCoefficient::DenseMask splatRegionfield(const Container::Regionfield& regionfield) const {
		namespace Splt = Generator::Regionfield::Splatting;
		return Generator::Regionfield::splat([this] noexcept -> Splt::Option {
			const auto option_oc = [splat_info = &this->OCSplatInfo](
				const auto option) constexpr noexcept -> variant_alternative_t<0U, Splt::Option> {
				return { splat_info, option };
			};

			using enum Splatting;
			switch (this->Splatting_) {
			case Full: return option_oc(Splt::OccupancyConvolution::Full {});
			case Stochastic: return option_oc(&this->StochasticSampled);
			case Stratified: return option_oc(&this->StratifiedSampled);
			case Systematic: return option_oc(&this->SystematicSampled);
			default: std::unreachable();
			}
		}(), regionfield);
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
	const auto& [output_file, _1, _2, _3] = arg_regionfield;

	RegionfieldProtocol::initialise();
	const auto regionfield_tif = Image::Tiff(output_file, "w");
	arg_tiff_compression.setCompression(regionfield_tif);
	RegionfieldProtocol::write(regionfield_tif, arg_regionfield.generateRegionfield(), arg_regionfield.seed());
}

void splatRegionfield(const Argument::Splat& arg_splat, const Argument::TiffCompression& arg_tiff_compression) {
	const auto& [regionfield_file, output_file, _1, _2, _3, _4, _5] = arg_splat;

	RegionfieldProtocol::initialise();
	const auto regionfield_tif = Image::Tiff(regionfield_file, "r");
	Container::Regionfield regionfield;
	RegionfieldProtocol::read(regionfield_tif, regionfield);

	const auto dense_mask_tif = Image::Tiff(output_file, "w");
	arg_tiff_compression.setCompression(dense_mask_tif);
	DenseMaskProtocol::write(dense_mask_tif, arg_splat.splatRegionfield(regionfield));
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

	CLI::App& cmd_profile = *parser.add_subcommand(
		"profile",
		"Initiate the profiler and then measure the execution time of various splatting implementations."
	);
	Argument::Profile arg_profile;
	arg_profile.bind(cmd_profile);

	CLI::App& group_tiff_compression = *parser.add_option_group(
		"tiff-compression",
		"Exercise control over the compression of generated data for the TIFF writer."
	)	->disabled_by_default()
		->require_option(-1);
	Argument::TiffCompression arg_tiff_compression;
	arg_tiff_compression.bind(group_tiff_compression);

	CLI::App& group_generator = *parser.add_option_group(
		"generator",
		format("The generation and processing of region data is to be carried out in a procedural manner, with the purpose of visually demonstrating the functionality of {} in the context of region splatting.", Info::FullName)
	)	->fallthrough();

	CLI::App& cmd_regionfield = *group_generator.add_subcommand(
		"regionfield",
		"A built-in regionfield generator is utilised to generate a regionfield matrix, which must then be stored as a TIFF image."
	);
	CLI::TriggerOn(&cmd_regionfield, &group_tiff_compression);
	Argument::Regionfield arg_regionfield;
	arg_regionfield.bind(cmd_regionfield);

	CLI::App& cmd_splat = *group_generator.add_subcommand(
		"splat",
		"Given a regionfield matrix, the region feature splatting mask is computed and saved as a TIFF image."
	);
	CLI::TriggerOn(&cmd_splat, &group_tiff_compression);
	Argument::Splat arg_splat;
	arg_splat.bind(cmd_splat);

	CLI11_PARSE(parser, argc, argv);

	if (cmd_profile) {
		runProfiler(arg_profile);
	} else if (cmd_regionfield) {
		generateRegionfield(arg_regionfield, arg_tiff_compression);
	} else if (cmd_splat) {
		splatRegionfield(arg_splat, arg_tiff_compression);
	}
	return EXIT_SUCCESS;
} catch (const exception& e) {
	DisRegRep::Core::Exception::print(e);
	return EXIT_FAILURE;
}