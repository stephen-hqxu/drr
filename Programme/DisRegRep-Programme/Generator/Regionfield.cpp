#include <DisRegRep-Programme/Generator/Regionfield.hpp>

#include <DisRegRep/Container/Regionfield.hpp>

#include <DisRegRep/RegionfieldGenerator/Base.hpp>
#include <DisRegRep/RegionfieldGenerator/ExecutionPolicy.hpp>
#include <DisRegRep/RegionfieldGenerator/Uniform.hpp>
#include <DisRegRep/RegionfieldGenerator/VoronoiDiagram.hpp>

#include <tuple>
#include <variant>

#include <functional>
#include <utility>

#include <concepts>

namespace RfGen = DisRegRep::Programme::Generator::Regionfield;
namespace StockGen = DisRegRep::RegionfieldGenerator;
using DisRegRep::Container::Regionfield;

using std::tuple, std::visit;
using std::invoke;
using std::derived_from;

namespace {

using PreparedGenerationInfo = tuple<Regionfield, const StockGen::Base::GenerateInfo>;

[[nodiscard]] Regionfield generate(
	//NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
	const derived_from<StockGen::Base> auto& generator, PreparedGenerationInfo&& prepared_generation_info) {
	auto& [regionfield, stock_gen_info] = prepared_generation_info;
	invoke(generator, StockGen::ExecutionPolicy::MultiThreadingTrait, regionfield, stock_gen_info);
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

}

Regionfield RfGen::generate(const GenerateInfo& gen_info, const Generator::Option& option) {
	const auto [resolution, region_count, seed] = gen_info;
	Container::Regionfield regionfield;
	regionfield.resize(resolution);
	regionfield.RegionCount = region_count;

	return visit([
		&regionfield,
		stock_gen_info = StockGen::Base::GenerateInfo {
			.Seed = seed
		}
	](const auto& generator) { return ::generate(PreparedGenerationInfo(std::move(regionfield), stock_gen_info), generator); }, option);
}