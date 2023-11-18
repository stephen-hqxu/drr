#include <DisRegRep/Factory/VoronoiRegionFactory.hpp>
#include <DisRegRep/Container/FixedHeapArray.hpp>

#include <nb/nanobench.h>

#include <execution>
#include <algorithm>
#include <ranges>
#include <numeric>

#include <iterator>
#include <utility>

#include <cmath>

using std::ranges::generate, std::min_element, std::for_each,
	std::views::iota, std::views::transform,
	std::execution::par_unseq, std::execution::unseq;
using std::as_const;

using ankerl::nanobench::Rng;

using namespace DisRegRep;
using namespace Format;

namespace {

float l2Distance(const SizeVec2& a, const SizeVec2& b) {
	const auto it = std::views::zip(a, b) | transform([](const auto ab) constexpr noexcept {
		const auto& [a, b] = ab;
		return a - b;
	});
	return std::sqrt(std::inner_product(it.cbegin(), it.cend(), it.cbegin(), 0.0f));
}

}

void VoronoiRegionFactory::operator()(const CreateDescription& desc, RegionMap& output) const {
	const auto [region_count] = desc;
	const auto [dim_x, dim_y] = output.dimension();
	output.RegionCount = region_count;

	auto rng = Rng(this->RandomSeed);
	//generate all the centroids with random region assignment
	auto region_centroid = FixedHeapArray<SizeVec2>(this->CentroidCount);
	auto region_assignment = FixedHeapArray<Region_t>(this->CentroidCount);
	
	generate(region_centroid,
		[&rng, x = static_cast<uint32_t>(dim_x), y = static_cast<uint32_t>(dim_y)]() noexcept {
			return SizeVec2 {
				rng.bounded(x),
				rng.bounded(y)
			};
		});
	generate(region_assignment, [&rng, rc = static_cast<uint32_t>(region_count)]() {
		return static_cast<Region_t>(rng.bounded(rc));
	});

	//find the nearest centroid for every point
	//This is a pretty naive algorithm,
	//	in practice it is better to do with either Quad-Tree or KD-Tree to find K-NN;
	//	omitted here for simplicity.
	const auto y_it = iota(size_t { 0 }, dim_y);
	for_each(par_unseq, y_it.cbegin(), y_it.cend(),
		[dim_x, &map = output, &rc = as_const(region_centroid), &ra = as_const(region_assignment)](const auto y) {
			for (const auto x : iota(size_t { 0 }, dim_x)) {
				const auto it = rc | transform([curr_position = SizeVec2 { x, y }](const auto& centroid) {
					return ::l2Distance(curr_position, centroid);
				});
				const auto min_idx = std::distance(
					it.cbegin(),
					min_element(unseq, it.cbegin(), it.cend())
				);

				map(x, y) = ra[min_idx];
			}
		});
}