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

#include <DisRegRep/Splatting/OccupancyConvolution/Full/Fast.hpp>
#include <DisRegRep/Splatting/OccupancyConvolution/Full/Vanilla.hpp>
#include <DisRegRep/Splatting/OccupancyConvolution/Base.hpp>
#include <DisRegRep/Splatting/Base.hpp>

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

namespace Drv = DisRegRep::Programme::Profiler::Driver;
namespace View = DisRegRep::Core::View;
namespace Splt = DisRegRep::Splatting;

namespace fs = std::filesystem;
using std::array, std::to_array, std::span, std::vector;
using std::string_view;
using std::tuple, std::tie, std::apply;
using std::ranges::to, std::ranges::copy,
	std::views::transform, std::views::zip, std::views::repeat;
using std::chrono::system_clock, std::chrono::sys_seconds, std::chrono::time_point_cast;
using std::format,
	std::index_sequence, std::make_index_sequence;
using std::throw_with_nested,
	std::make_error_code, std::errc;
using std::derived_from,
	std::type_identity, std::add_const_t, std::add_pointer_t, std::integral_constant;

namespace {

template<std::size_t Value>
using IndexConstant = integral_constant<std::size_t, Value>;

template<derived_from<Splt::OccupancyConvolution::Base> OccuConv>
[[nodiscard]] constexpr auto getRadiusSweepSplatting(const auto& variable_radius) {
	return apply(View::Arithmetic::LinSpace, variable_radius) | transform([](const auto radius) static constexpr noexcept {
		OccuConv oc_splatting;
		oc_splatting.Radius = radius;
		return oc_splatting;
	}) | to<vector>();
}

template<typename... OccuConv>
requires(derived_from<OccuConv, Splt::OccupancyConvolution::Base> && ...)
[[nodiscard]] constexpr auto getAllRadiusSweepSplatting(const auto& variable_radius) {
	return apply([&variable_radius]<typename... CurrentOccuConv>(type_identity<CurrentOccuConv>...) constexpr {
		return tuple(getRadiusSweepSplatting<CurrentOccuConv>(variable_radius)...);
	}, tuple<type_identity<OccuConv>...> {});
}

template<typename... OccuConv>
requires(derived_from<OccuConv, Splt::OccupancyConvolution::Base> && ...)
[[nodiscard]] constexpr auto viewOccupancyConvolution(const tuple<vector<OccuConv>...>& occupancy_convolution) {
	return apply([](const auto&... occu_conv) static constexpr {
		vector<const Splt::OccupancyConvolution::Base*> occu_conv_view;
		occu_conv_view.reserve((occu_conv.size() + ...));
		(occu_conv_view.append_range(occu_conv | View::Functional::AddressOf), ...);
		return occu_conv_view;
	}, occupancy_convolution);
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

}

void Drv::splatting(const SplattingInfo& info) {
	const auto [result_dir, thread_pool_create_info, bg_thread_affinity_mask, seed, progress_log, parameter_set] = info;
	const auto& [default_profile, stress_profile] = *parameter_set;
	const auto& [default_fixed, default_variable] = default_profile;
	const auto& [stress_fixed, stress_variable] = stress_profile;

	using Container::Regionfield,
		RegionfieldGenerator::Uniform,
		RegionfieldGenerator::VoronoiDiagram,
		Splt::OccupancyConvolution::Full::Fast,
		Splt::OccupancyConvolution::Full::Vanilla;

	const tuple default_variable_radius = [&default_variable] {
		const auto& [variable_radius, _1, _2] = default_variable;
		return getAllRadiusSweepSplatting<Vanilla, Fast>(variable_radius);
	}();
	const vector default_variable_radius_ptr = viewOccupancyConvolution(default_variable_radius);
	const auto [default_variable_region_count, default_variable_centroid_count] = [&default_variable] {
		const auto& [_, variable_region_count, variable_centroid_count] = default_variable;
		return apply([](const auto&... variable) static constexpr {
			return tuple(apply(View::Arithmetic::LinSpace, variable) | to<vector>()...);
		}, tie(variable_region_count, variable_centroid_count));
	}();
	const tuple stress_variable_radius = [&stress_variable] {
		const auto& [variable_radius] = stress_variable;
		return getAllRadiusSweepSplatting<Fast>(variable_radius);
	}();
	const vector stress_variable_radius_ptr = viewOccupancyConvolution(stress_variable_radius);

	const tuple default_rf_gen = [centroid_count = default_fixed.CentroidCount] constexpr noexcept {
		tuple<Uniform, VoronoiDiagram> rf_gen;
		std::get<VoronoiDiagram>(rf_gen).CentroidCount = centroid_count;
		return rf_gen;
	}();
	const array default_rf_gen_ptr = viewTuple<RegionfieldGenerator::Base>(default_rf_gen);
	const auto [_, default_voronoi_rf_gen_ptr] = default_rf_gen_ptr;
	static constexpr tuple<Uniform> stress_rf_gen;
	const array stress_rf_gen_ptr = viewTuple<RegionfieldGenerator::Base>(stress_rf_gen);

	static constexpr auto make_regionfield = []<std::size_t Repetition>(
												 IndexConstant<Repetition>, const auto& fixed) static constexpr -> auto {
		return [region_count = fixed.RegionCount]<std::size_t... I>(index_sequence<I...>) constexpr {
			array<Regionfield, Repetition> rf;
			((rf[I].RegionCount = region_count), ...);
			return rf;
		}(make_index_sequence<Repetition> {});
	};
	array default_rf = make_regionfield(IndexConstant<2U> {}, default_fixed);
	array stress_rf = make_regionfield(IndexConstant<1U> {}, stress_fixed);
	const array default_rf_ptr = viewArray(default_rf);
	const array stress_rf_ptr = viewArray(stress_rf);

	const tuple default_fixed_radius = [radius = default_fixed.Radius] constexpr noexcept {
		tuple<Vanilla, Fast> splatting;
		apply([radius](auto&... current_splatting) constexpr noexcept { ((current_splatting.Radius = radius), ...); }, splatting);
		return splatting;
	}();
	const array default_fixed_radius_ptr = viewTuple<Splt::Base>(default_fixed_radius);

	const RegionfieldGenerator::Base::GenerateInfo rf_gen_info {
		.Seed = seed
	};
	const auto [default_common_info, stress_common_info] = [
		extent = apply(
			[](const auto&... fixed) static constexpr noexcept { return array { fixed.Extent... }; },
			tie(default_fixed, stress_fixed)
		),
		rf_gen_info = repeat(&rf_gen_info),
		tag = to_array<string_view>({ "Default", "Stress" })
	] constexpr {
		array<Splatting::CommonSweepInfo, extent.size()> common_info;
		copy(zip(tag, rf_gen_info, extent) | View::Functional::MakeFromTuple<Splatting::CommonSweepInfo>, common_info.begin());
		return common_info;
	}();

	try {
		DRR_ASSERT(fs::is_directory(*result_dir));
	} catch (const Core::Exception& e) {
		throw_with_nested(fs::filesystem_error(e.what(), *result_dir, make_error_code(errc::not_a_directory)));
	}
	const fs::path output_dir = *result_dir / format("{:%Y-%m-%d_%H-%M-%S}",
		time_point_cast<sys_seconds::duration>(system_clock::now()));
	DRR_ASSERT(fs::create_directory(output_dir));

	namespace ProcThrCtrl = Core::System::ProcessThreadControl;
	[[maybe_unused]] const volatile class MainThreadScheduler {
	private:

		const ProcThrCtrl::Priority Priority = ProcThrCtrl::getPriority();
		const ProcThrCtrl::AffinityMask AffinityMask = ProcThrCtrl::getAffinityMask();

	public:

		MainThreadScheduler(const ProcThrCtrl::AffinityMask affinity_mask) {
			ProcThrCtrl::setPriority(ProcThrCtrl::PriorityPreset::Min);
			ProcThrCtrl::setAffinityMask(affinity_mask);
		}

		~MainThreadScheduler() noexcept(false) {
			ProcThrCtrl::setPriority(this->Priority);
			ProcThrCtrl::setAffinityMask(this->AffinityMask);
		}

	} main_thread_scheduler = bg_thread_affinity_mask;

	const auto profiler = Splatting(output_dir, *thread_pool_create_info);
	profiler.sweepRadius(default_variable_radius_ptr, std::get<2U>(default_variable.Radius), {
		.CommonSweepInfo_ = &default_common_info,
		.Input = {
			default_rf_gen_ptr,
			default_rf_ptr
		}
	});
	profiler.sweepRadius(stress_variable_radius_ptr, std::get<2U>(stress_variable.Radius), {
		.CommonSweepInfo_ = &stress_common_info,
		.Input = {
			stress_rf_gen_ptr,
			stress_rf_ptr
		}
	});
	profiler.sweepRegionCount(default_fixed_radius_ptr, default_variable_region_count, {
		.CommonSweepInfo_ = &default_common_info,
		.Input = span(&default_voronoi_rf_gen_ptr, 1U)
	});
	profiler.sweepCentroidCount(default_fixed_radius_ptr, default_variable_centroid_count, {
		.CommonSweepInfo_ = &default_common_info,
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