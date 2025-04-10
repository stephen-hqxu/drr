#include <DisRegRep-Programme/Generator/Regionfield.hpp>

#include <DisRegRep/Container/Regionfield.hpp>
#include <DisRegRep/Container/SplattingCoefficient.hpp>

#include <DisRegRep/RegionfieldGenerator/Base.hpp>
#include <DisRegRep/RegionfieldGenerator/ExecutionPolicy.hpp>
#include <DisRegRep/RegionfieldGenerator/Uniform.hpp>
#include <DisRegRep/RegionfieldGenerator/VoronoiDiagram.hpp>

#include <DisRegRep/Splatting/OccupancyConvolution/Full/Fast.hpp>
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

using PreparedGenerationInfo = tuple<Regionfield, const StockGen::Base::GenerateInfo>;
using PreparedSplattingInfo = tuple<const Regionfield&>;

[[nodiscard]] Regionfield generate(
	//NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
	const StockGen::Base& generator, PreparedGenerationInfo&& prepared_gen_info) {
	auto& [regionfield, stock_gen_info] = prepared_gen_info;

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

[[nodiscard]] DenseMask splat(
	//NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
	const StockSplt::Base& splatting, PreparedSplattingInfo&& prepared_splat_info) {
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

[[nodiscard]] DenseMask splat(PreparedSplattingInfo&& prepared_splat_info, const RfGen::Splatting::FullOccupancyConvolution option) {
	const auto [radius] = option;
	StockSplt::OccupancyConvolution::Full::Fast fast_occupancy;
	fast_occupancy.Radius = radius;
	return splat(fast_occupancy, std::move(prepared_splat_info));
}

}

Regionfield RfGen::generate(const GenerateInfo& gen_info, const Generator::Option& option) {
	const auto [resolution, region_count, stock_gen_info] = gen_info;
	Container::Regionfield regionfield;
	regionfield.resize(resolution);
	regionfield.RegionCount = region_count;

	return visit(
		[&](const auto& generator) { return ::generate(PreparedGenerationInfo(
			std::move(regionfield),
			stock_gen_info
		), generator); },
		option
	);
}

DenseMask RfGen::splat(const Splatting::Option& option, const Container::Regionfield& regionfield) {
	return visit([&](const auto& splatting) { return ::splat(PreparedSplattingInfo(regionfield), splatting); }, option);
}