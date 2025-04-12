#include <DisRegRep-Programme/Generator/Regionfield.hpp>

#include <DisRegRep/Container/Regionfield.hpp>
#include <DisRegRep/Container/SplattingCoefficient.hpp>

#include <DisRegRep/RegionfieldGenerator/Base.hpp>
#include <DisRegRep/RegionfieldGenerator/ExecutionPolicy.hpp>
#include <DisRegRep/RegionfieldGenerator/Uniform.hpp>
#include <DisRegRep/RegionfieldGenerator/VoronoiDiagram.hpp>

#include <DisRegRep/Splatting/OccupancyConvolution/Full/Fast.hpp>
#include <DisRegRep/Splatting/OccupancyConvolution/Sampled/Stochastic.hpp>
#include <DisRegRep/Splatting/OccupancyConvolution/Sampled/Stratified.hpp>
#include <DisRegRep/Splatting/OccupancyConvolution/Sampled/Systematic.hpp>
#include <DisRegRep/Splatting/OccupancyConvolution/Base.hpp>
#include <DisRegRep/Splatting/Base.hpp>
#include <DisRegRep/Splatting/Container.hpp>

#include <any>
#include <tuple>
#include <variant>

#include <utility>

namespace RfGen = DisRegRep::Programme::Generator::Regionfield;
namespace StockGen = DisRegRep::RegionfieldGenerator;
namespace StockSplt = DisRegRep::Splatting;
using DisRegRep::Container::Regionfield, DisRegRep::Container::SplattingCoefficient::DenseMask;

using std::any, std::tuple, std::visit;

namespace {

using PreparedGenerationInfo = tuple<const StockGen::Base::GenerateInfo, Regionfield>;
using PreparedSplatInfo = tuple<const Regionfield&>;
using PreparedOccupancyConvolutionSplatInfo = tuple<const RfGen::Splatting::OccupancyConvolution::SplatInfo, PreparedSplatInfo>;

[[nodiscard]] Regionfield generate(
	//NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
	const StockGen::Base& generator, PreparedGenerationInfo&& prepared_gen_info) {
	auto& [stock_gen_info, regionfield] = prepared_gen_info;

	generator(StockGen::ExecutionPolicy::MultiThreadingTrait, regionfield, stock_gen_info);
	return std::move(regionfield);
}

[[nodiscard]] Regionfield generate(PreparedGenerationInfo&& prepared_gen_info, RfGen::Generator::Uniform) {
	static constexpr StockGen::Uniform Uniform;
	return generate(Uniform, std::move(prepared_gen_info));
}
[[nodiscard]] Regionfield generate(PreparedGenerationInfo&& prepared_gen_info, const RfGen::Generator::VoronoiDiagram option) {
	const auto [centroid_count] = option;
	StockGen::VoronoiDiagram voronoi_diagram;
	voronoi_diagram.CentroidCount = centroid_count;
	return generate(voronoi_diagram, std::move(prepared_gen_info));
}

//NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
[[nodiscard]] DenseMask splat(const StockSplt::Base& splatting, PreparedSplatInfo&& prepared_splat_info) {
	const auto invoke_splat = [&splatting](const Regionfield& regionfield) -> DenseMask {
		any memory;
		const StockSplt::Base::DimensionType offset = splatting.minimumOffset();
		return std::move(splatting(StockSplt::Container::DenseKernelDenseOutputTrait, StockSplt::Base::InvokeInfo {
			.Offset = offset,
			.Extent = splatting.maximumExtent(regionfield, offset)
		}, regionfield, memory));
	};

	//Remember to transpose the input to maintain the same axes order if the splatting algorithm would do so.
	if (const auto& [regionfield] = prepared_splat_info;
		splatting.isTransposed()) {
		return invoke_splat(regionfield.transpose());
	} else {//NOLINT(readability-else-after-return)
		return invoke_splat(regionfield);
	}
}

[[nodiscard]] DenseMask splat(
	StockSplt::OccupancyConvolution::Base& splatting, PreparedOccupancyConvolutionSplatInfo&& prepared_oc_splat_info) {
	auto [splat_info, prepared_splat_info] = std::move(prepared_oc_splat_info);
	const auto [radius] = splat_info;
	splatting.Radius = radius;
	return splat(splatting, std::move(prepared_splat_info));
}

[[nodiscard]] DenseMask splat(
	PreparedOccupancyConvolutionSplatInfo&& prepared_oc_splat_info, RfGen::Splatting::OccupancyConvolution::Full) {
	StockSplt::OccupancyConvolution::Full::Fast full;
	return splat(full, std::move(prepared_oc_splat_info));
}
[[nodiscard]] DenseMask splat(PreparedOccupancyConvolutionSplatInfo&& prepared_oc_splat_info,
	const RfGen::Splatting::OccupancyConvolution::Sampled::Stochastic* const option) {
	const auto [sample, seed] = *option;
	StockSplt::OccupancyConvolution::Sampled::Stochastic stochastic;
	stochastic.Sample = sample;
	stochastic.Seed = seed;
	return splat(stochastic, std::move(prepared_oc_splat_info));
}
[[nodiscard]] DenseMask splat(PreparedOccupancyConvolutionSplatInfo&& prepared_oc_splat_info,
	const RfGen::Splatting::OccupancyConvolution::Sampled::Stratified* const option) {
	const auto [stratum_count, seed] = *option;
	StockSplt::OccupancyConvolution::Sampled::Stratified stratified;
	stratified.StratumCount = stratum_count;
	stratified.Seed = seed;
	return splat(stratified, std::move(prepared_oc_splat_info));
}
[[nodiscard]] DenseMask splat(PreparedOccupancyConvolutionSplatInfo&& prepared_oc_splat_info,
	const RfGen::Splatting::OccupancyConvolution::Sampled::Systematic* const option) {
	const auto [first_sample, interval] = *option;
	StockSplt::OccupancyConvolution::Sampled::Systematic systematic;
	systematic.FirstSample = first_sample;
	systematic.Interval = interval;
	return splat(systematic, std::move(prepared_oc_splat_info));
}

}

Regionfield RfGen::generate(const GenerateInfo& gen_info, const Generator::Option& option) {
	const auto [resolution, region_count, stock_gen_info] = gen_info;
	Container::Regionfield regionfield;
	regionfield.resize(resolution);
	regionfield.RegionCount = region_count;

	return visit(
		[&](const auto& generator) { return ::generate(PreparedGenerationInfo(
			stock_gen_info,
			std::move(regionfield)
		), generator); },
		option
	);
}

DenseMask RfGen::splat(const Splatting::Option& option, const Container::Regionfield& regionfield) {
	return visit(
		[&](const auto& splatting_group) {
			const auto& [splat_info, option] = splatting_group;
			return visit(
				[&](const auto& splatting) {
					return ::splat(PreparedOccupancyConvolutionSplatInfo(
						*splat_info,
						PreparedSplatInfo(regionfield)
					), splatting);
				},
				option
			);
		},
		option
	);
}