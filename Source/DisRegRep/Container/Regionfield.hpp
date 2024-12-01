#pragma once

#include <DisRegRep/Type.hpp>

#include <mdspan>
#include <vector>

#include <cstdint>

namespace DisRegRep::Container {

/**
 * @brief Regionfield is a function that tells which region a given point belongs to. In this project regionfield is constructed as a
 * 2D matrix of region identifiers. The storage uses column-major order.
 */
class Regionfield {
public:

	using ValueType = Type::RegionIdentifier;
	using IndexType = std::uint16_t;

private:

	using MdSpanType = std::mdspan<ValueType, std::dextents<IndexType, 2U>, std::layout_left>;

	std::vector<ValueType> Data;
	MdSpanType View;

public:

	/**
	 * @brief The total number of regions that are supposed to present in the regionfield. Note that it is likely that not all regions
	 * are on the regionfield.
	 */
	Type::RegionIdentifier RegionCount {};

	constexpr Regionfield() = default;

	Regionfield(const Regionfield&) = delete;

	Regionfield(Regionfield&&) = delete;

	Regionfield& operator=(const Regionfield&) = delete;

	Regionfield& operator=(Regionfield&&) = delete;

	~Regionfield() = default;

	/**
	 * @brief Get the region identifier by matrix indices.
	 *
	 * @param x, y The matrix indices into the regionfield.
	 *
	 * @return The region identifier.
	 */
	[[nodiscard]] constexpr ValueType& operator[](const auto x, const auto y) noexcept {
		return this->View[x, y];
	}
	[[nodiscard]] constexpr ValueType operator[](const auto x, const auto y) const noexcept {
		return this->View[x, y];
	}

	/**
	 * @brief Reshape regionfield matrix to a new dimension.
	 *
	 * @param dim The new dimension of the regionfield matrix.
	 * If dimension is shrunken, the matrix will be trimmed linearly.
	 */
	void reshape(Type::SizeVec2);

	/**
	 * @brief Get the linear size of the regionfield matrix.
	 *
	 * @return The total number of region identifiers stored.
	 */
	[[nodiscard]] constexpr auto size() const noexcept {
		return this->Data.size();
	}

	/**
	 * @brief Get the dimension of the regionfield matrix.
	 *
	 * @return The dimension.
	 */
	[[nodiscard]] Type::SizeVec2 dimension() const noexcept;

	[[nodiscard]] constexpr auto begin() noexcept {
		return this->Data.begin();
	}
	[[nodiscard]] constexpr auto end() noexcept {
		return this->Data.end();
	}

};

}