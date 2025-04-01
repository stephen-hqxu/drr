#include <DisRegRep-Programme/Generator/Regionfield.hpp>

#include <DisRegRep/Container/Regionfield.hpp>

#include <DisRegRep/RegionfieldGenerator/Base.hpp>
#include <DisRegRep/RegionfieldGenerator/ExecutionPolicy.hpp>
#include <DisRegRep/RegionfieldGenerator/Uniform.hpp>
#include <DisRegRep/RegionfieldGenerator/VoronoiDiagram.hpp>

#include <tuple>

#include <functional>
#include <utility>

#include <concepts>

namespace RfGen = DisRegRep::Programme::Generator::Regionfield;
namespace Ctn = DisRegRep::Container;
namespace StockGen = DisRegRep::RegionfieldGenerator;

using std::tuple;
using std::invoke;
using std::derived_from;

namespace {

using PreparedGenerationInfo = tuple<Ctn::Regionfield, const StockGen::Base::GenerateInfo>;

[[nodiscard]] PreparedGenerationInfo prepareRegionfieldGeneration(const RfGen::GenerateInfo& gen_info) {
	const auto [resolution, region_count, seed] = gen_info;
	Ctn::Regionfield regionfield;
	regionfield.resize(resolution);
	regionfield.RegionCount = region_count;
	return tuple(
		std::move(regionfield),
		StockGen::Base::GenerateInfo {
			.Seed = seed
		}
	);
}

//NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
Ctn::Regionfield generate(const derived_from<StockGen::Base> auto& generator, PreparedGenerationInfo&& prepared_generation_info) {
	auto& [regionfield, stock_gen_info] = prepared_generation_info;
	invoke(generator, StockGen::ExecutionPolicy::MultiThreadingTrait, regionfield, stock_gen_info);
	return std::move(regionfield);
}

}

Ctn::Regionfield RfGen::uniform(const GenerateInfo& gen_info) {
	static constexpr StockGen::Uniform Uniform;
	return generate(Uniform, prepareRegionfieldGeneration(gen_info));
}

Ctn::Regionfield RfGen::voronoiDiagram(const GenerateInfo& gen_info, const StockGen::VoronoiDiagram::SizeType centroid_count) {
	StockGen::VoronoiDiagram voronoi_diagram;
	voronoi_diagram.CentroidCount = centroid_count;
	return generate(voronoi_diagram, prepareRegionfieldGeneration(gen_info));
}