#include <DisRegRep-Programme/Profiler/Driver.hpp>
#include <DisRegRep-Programme/Profiler/Splatting.hpp>
#include <DisRegRep-Programme/YamlConverter.hpp>

#include <DisRegRep/Container/Regionfield.hpp>

#include <DisRegRep/Core/System/ProcessThreadControl.hpp>
#include <DisRegRep/Core/View/Arithmetic.hpp>
#include <DisRegRep/Core/View/Functional.hpp>
#include <DisRegRep/Core/Exception.hpp>

#include <DisRegRep/RegionfieldGenerator/Base.hpp>
#include <DisRegRep/RegionfieldGenerator/Uniform.hpp>
#include <DisRegRep/RegionfieldGenerator/VoronoiDiagram.hpp>

#include <DisRegRep/Splatting/Convolution/Full/FastOccupancy.hpp>
#include <DisRegRep/Splatting/Convolution/Full/VanillaOccupancy.hpp>

#include <yaml-cpp/yaml.h>

#include <array>
#include <span>
#include <vector>

#include <string_view>

#include <tuple>

#include <algorithm>
#include <ranges>

#include <chrono>
#include <format>
#include <memory>
#include <utility>

#include <filesystem>

#include <exception>
#include <system_error>

#include <concepts>
#include <type_traits>

#include <cstddef>

namespace Prof = DisRegRep::Programme::Profiler;
namespace Drv = Prof::Driver;
namespace View = DisRegRep::Core::View;

namespace crn = std::chrono;
namespace fs = std::filesystem;
using std::array, std::to_array, std::span, std::vector;
using std::string_view;
using std::tuple, std::tie, std::apply;
using std::ranges::to, std::ranges::copy,
	std::views::transform, std::views::zip;
using std::format,
	std::index_sequence, std::make_index_sequence;
using std::throw_with_nested,
	std::make_error_code, std::errc;
using std::derived_from,
	std::type_identity, std::add_const_t, std::add_pointer_t;

namespace {

template<derived_from<Prof::Splatting::BaseConvolution> Conv>
[[nodiscard]] constexpr auto getRadiusSweepSplatting(const auto& variable_radius) {
	return apply(View::Arithmetic::LinSpace, variable_radius) | transform([](const auto radius) static constexpr noexcept {
		Conv convolution_splatting;
		convolution_splatting.Radius = radius;
		return convolution_splatting;
	}) | to<vector>();
}

template<typename... Conv>
requires(derived_from<Conv, Prof::Splatting::BaseConvolution> && ...)
[[nodiscard]] constexpr auto getAllRadiusSweepSplatting(const auto& variable_radius) {
	return apply([&variable_radius]<typename... CurrentConv>(type_identity<CurrentConv>...) constexpr {
		return tuple(getRadiusSweepSplatting<CurrentConv>(variable_radius)...);
	}, tuple<type_identity<Conv>...> {});
}

template<typename... Conv>
requires(derived_from<Conv, Prof::Splatting::BaseConvolution> && ...)
[[nodiscard]] constexpr auto viewConvolution(const tuple<vector<Conv>...>& convolution) {
	return apply([](const auto&... conv) static constexpr {
		vector<const Prof::Splatting::BaseConvolution*> conv_view;
		conv_view.reserve((conv.size() + ...));
		(conv_view.append_range(conv | View::Functional::AddressOf), ...);
		return conv_view;
	}, convolution);
}

template<typename Dst, typename... Src>
requires(derived_from<Src, Dst> && ...)
[[nodiscard]] constexpr auto viewTuple(const tuple<Src...>& src) noexcept {
	return apply([](const auto&... elem) static constexpr noexcept {
		return to_array<add_pointer_t<add_const_t<Dst>>>({ std::addressof(elem)... });
	}, src);
}

template<typename T, std::size_t S>
[[nodiscard]] constexpr auto viewArray(array<T, S>& a) noexcept {
	return apply([](auto&... elem) static constexpr noexcept { return array { std::addressof(elem)... }; }, a);
}

template<typename... RfGen>
requires(derived_from<RfGen, DisRegRep::RegionfieldGenerator::Base> && ...)
constexpr void seedGenerator(tuple<RfGen...>& generator, const Prof::Splatting::SeedType seed) noexcept {
	apply([seed](auto&... current_generator) constexpr noexcept { ((current_generator.Seed = seed), ...); }, generator);
}

}

void Drv::splatting(const SplattingInfo& info) {
	const auto [result_dir, thread_pool_create_info, seed, progress_log, parameter_set] = info;
	const auto& [default_profile, stress_profile] = *parameter_set;
	const auto& [default_fixed, default_variable] = default_profile;
	const auto& [stress_fixed, stress_variable] = stress_profile;

	using Container::Regionfield,
		RegionfieldGenerator::Uniform,
		RegionfieldGenerator::VoronoiDiagram,
		DisRegRep::Splatting::Convolution::Full::FastOccupancy,
		DisRegRep::Splatting::Convolution::Full::VanillaOccupancy;

	const tuple default_variable_radius = [&default_variable] {
		const auto& [variable_radius, _1, _2] = default_variable;
		return getAllRadiusSweepSplatting<VanillaOccupancy, FastOccupancy>(variable_radius);
	}();
	const vector default_variable_radius_ptr = viewConvolution(default_variable_radius);
	const auto [default_variable_region_count, default_variable_centroid_count] = [&default_variable] {
		const auto& [_, variable_region_count, variable_centroid_count] = default_variable;
		return apply([](const auto&... variable) static constexpr {
			return tuple(apply(View::Arithmetic::LinSpace, variable) | to<vector>()...);
		}, tie(variable_region_count, variable_centroid_count));
	}();
	const tuple stress_variable_radius = [&stress_variable] {
		const auto& [variable_radius] = stress_variable;
		return getAllRadiusSweepSplatting<FastOccupancy>(variable_radius);
	}();
	const vector stress_variable_radius_ptr = viewConvolution(stress_variable_radius);

	const tuple default_rf_gen = [seed, centroid_count = default_fixed.CentroidCount] constexpr noexcept {
		tuple<Uniform, VoronoiDiagram> rf_gen;
		seedGenerator(rf_gen, seed);
		std::get<VoronoiDiagram>(rf_gen).CentroidCount = centroid_count;
		return rf_gen;
	}();
	const array default_rf_gen_ptr = viewTuple<RegionfieldGenerator::Base>(default_rf_gen);
	const auto [_, default_voronoi_rf_gen_ptr] = default_rf_gen_ptr;
	const tuple stress_rf_gen = [seed] constexpr noexcept {
		tuple<Uniform> rf_gen;
		seedGenerator(rf_gen, seed);
		return rf_gen;
	}();
	const array stress_rf_gen_ptr = viewTuple<RegionfieldGenerator::Base>(stress_rf_gen);

	array rf = [
		region_count = apply(
			[](const auto&... fixed) static constexpr noexcept { return array { fixed.RegionCount... }; },
			tie(default_fixed, stress_fixed)
		)
	] constexpr {
		array<Regionfield, region_count.size()> rf;
		[&]<std::size_t... I>(index_sequence<I...>) constexpr {
			((rf[I].RegionCount = region_count[I]), ...);
		}(make_index_sequence<rf.size()> {});
		return rf;
	}();
	const auto [default_rf_ptr, stress_rf_ptr] = viewArray(rf);

	const tuple default_fixed_radius = [radius = default_fixed.Radius] constexpr noexcept {
		tuple<VanillaOccupancy, FastOccupancy> splatting;
		apply([radius](auto&... current_splatting) constexpr noexcept { ((current_splatting.Radius = radius), ...); }, splatting);
		return splatting;
	}();
	const array default_fixed_radius_ptr = viewTuple<Splatting::Base>(default_fixed_radius);

	const auto [default_common_info, stress_common_info] = [
		extent = apply(
			[](const auto&... fixed) static constexpr noexcept { return array { fixed.Extent... }; },
			tie(default_fixed, stress_fixed)
		),
		tag = to_array<string_view>({ "Default", "Stress" })
	] constexpr {
		array<Splatting::CommonSweepInfo, extent.size()> common_info;
		copy(zip(tag, extent) | View::Functional::MakeFromTuple<Splatting::CommonSweepInfo>, common_info.begin());
		return common_info;
	}();

	try {
		DRR_ASSERT(fs::is_directory(*result_dir));
	} catch (const Core::Exception& e) {
		throw_with_nested(fs::filesystem_error(e.what(), *result_dir, make_error_code(errc::not_a_directory)));
	}
	const fs::path output_dir = *result_dir / format("{:%Y-%m-%d_%H-%M}",
		crn::zoned_time(crn::current_zone(), crn::time_point_cast<crn::minutes>(crn::system_clock::now())));
	DRR_ASSERT(fs::create_directory(output_dir));

	namespace ProcThrCtrl = Core::System::ProcessThreadControl;
	[[maybe_unused]] const volatile class MainThreadScheduler {
	private:

		bool AffinityMasked = false;

	public:

		MainThreadScheduler(const ProcThrCtrl::AffinityMask splatting_thread_affinity_mask) {
			ProcThrCtrl::setPriority(ProcThrCtrl::PriorityPreset::Min);
			if (const ProcThrCtrl::AffinityMask main_thread_affinity_mask = ~splatting_thread_affinity_mask;
				main_thread_affinity_mask.any()) {
				ProcThrCtrl::setAffinityMask(main_thread_affinity_mask);
				this->AffinityMasked = true;
			}
		}

		~MainThreadScheduler() {
			ProcThrCtrl::setPriority();
			if (this->AffinityMasked) {
				ProcThrCtrl::setAffinityMask();
			}
		}

	} main_thread_scheduler = thread_pool_create_info->AffinityMask;

	const auto profiler = Splatting(output_dir, *thread_pool_create_info);
	profiler.sweepRadius(default_variable_radius_ptr, std::get<2U>(default_variable.Radius), {
		.CommonSweepInfo_ = &default_common_info,
		.Input = tuple(
			default_rf_gen_ptr,
			span(&default_rf_ptr, 1U)
		)
	});
	profiler.sweepRadius(stress_variable_radius_ptr, std::get<2U>(stress_variable.Radius), {
		.CommonSweepInfo_ = &stress_common_info,
		.Input = tuple(
			stress_rf_gen_ptr,
			span(&stress_rf_ptr, 1U)
		)
	});
	profiler.sweepRegionCount(default_fixed_radius_ptr, default_variable_region_count, {
		.CommonSweepInfo_ = &default_common_info,
		.Input = span(&default_voronoi_rf_gen_ptr, 1U)
	});
	profiler.sweepCentroidCount(default_fixed_radius_ptr, default_variable_centroid_count, {
		.CommonSweepInfo_ = &default_common_info,
		.Seed = seed,
		.RegionCount = default_fixed.RegionCount
	});
	profiler.synchronise(progress_log);
}

bool YAML::convert<Drv::SplattingInfo::ParameterSetType>::decode(const Node& node, ConvertType& parameter_set) {
	if (!node.IsMap()) [[unlikely]] {
		return false;
	}
	const Node &node_default = node["default"], &node_stress = node["stress"],
		&default_fixed = node_default["fixed"], &default_variable = node_default["variable"],
		&stress_fixed = node_stress["fixed"], &stress_variable = node_stress["variable"];

	using Splatting = DisRegRep::Programme::Profiler::Splatting;
	parameter_set = {
		.Default = {
			.Fixed = {
				.Extent = default_fixed["extent"].as<Splatting::DimensionType>(),
				.Radius = default_fixed["radius"].as<Splatting::KernelSizeType>(),
				.RegionCount = default_fixed["region count"].as<Splatting::RegionCountType>(),
				.CentroidCount = default_fixed["centroid count"].as<Splatting::CentroidCountType>()
			},
			.Variable = {
				.Radius = default_variable["radius"].as<Drv::LinearSweepVariable<Splatting::KernelSizeType>>(),
				.RegionCount = default_variable["region count"].as<Drv::LinearSweepVariable<Splatting::RegionCountType>>(),
				.CentroidCount = default_variable["centroid count"].as<Drv::LinearSweepVariable<Splatting::CentroidCountType>>()
			}
		},
		.Stress = {
			.Fixed = {
				.Extent = stress_fixed["extent"].as<Splatting::DimensionType>(),
				.RegionCount = stress_fixed["region count"].as<Splatting::RegionCountType>()
			},
			.Variable = {
				.Radius = stress_variable["radius"].as<Drv::LinearSweepVariable<Splatting::KernelSizeType>>()
			}
		}
	};
	return true;
}