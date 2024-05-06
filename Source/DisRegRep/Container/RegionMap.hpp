#pragma once

#include "../Format.hpp"
#include "../Maths/Indexer.hpp"

#include <vector>
#include <cstddef>

namespace DisRegRep {

/**
 * @brief A 2D matrix of regions.
*/
class RegionMap {
private:

	std::vector<Format::Region_t> Map;/**< Region map data. */
	Format::SizeVec2 Dimension { };/**< The dimension of region map. */

	using RegionMapIndexer_t = Indexer<0, 1>;
	RegionMapIndexer_t RegionMapIndexer;/**< Default indexing order of region map. */

public:

	Format::Region_t RegionCount { };/**< The total number of contiguous region presented on region map. */

	constexpr RegionMap() noexcept = default;

	RegionMap(const RegionMap&) = delete;

	constexpr RegionMap(RegionMap&&) noexcept = default;

	constexpr ~RegionMap() = default;

	/**
	 * @brief Get the region value by pixel coordinate.
	 *
	 * @param x, y The axis indices into the region map.
	 *
	 * @return The region value.
	*/
	constexpr Format::Region_t& operator[](const auto x, const auto y) noexcept {
		return this->Map[this->RegionMapIndexer[x, y]];
	}
	constexpr Format::Region_t operator[](const auto x, const auto y) const noexcept {
		return this->Map[this->RegionMapIndexer[x, y]];
	}

	/**
	 * @brief Reshape region map to new dimension.
	 *
	 * @param dimension The new dimension of region map.
	 * If dimension is shrunken, region map will be trimmed linearly.
	*/
	void reshape(const Format::SizeVec2&);

	/**
	 * @brief Get the size of region map.
	 *
	 * @return The total number of pixel this region map has.
	*/
	size_t size() const noexcept;

	/**
	 * @brief Get the dimension of region map.
	 *
	 * @return The dimension.
	*/
	constexpr const Format::SizeVec2& dimension() const noexcept {
		return this->Dimension;
	}

	////////////////////////////
	/// Container functions
	///////////////////////////

	constexpr auto begin() noexcept {
		return this->Map.begin();
	}
	constexpr auto end() noexcept {
		return this->Map.end();
	}

};

}