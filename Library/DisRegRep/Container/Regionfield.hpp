#pragma once

#include <DisRegRep/Core/View/Matrix.hpp>
#include <DisRegRep/Core/Type.hpp>
#include <DisRegRep/Core/UninitialisedAllocator.hpp>

#include <glm/vec2.hpp>

#include <mdspan>
#include <span>
#include <vector>

#include <ranges>

#include <cstdint>

namespace DisRegRep::Container {

/**
 * @brief The regionfield function is employed to determine the region to which a given point belongs. In this project, regionfield is
 * constructed as a 2D matrix $s_{W,H}$ of region identifiers, where $W$ and $H$ denote the width and height of the matrix,
 * respectively. The expression $s[r,c]$ is the region identifier at row $r$ and column $c$, and the column has a stride of one.
 */
class Regionfield {
public:

	using ValueType = Core::Type::RegionIdentifier;
	using ConstValue = std::add_const_t<ValueType>;
	using IndexType = std::uint_fast32_t;
	using DimensionType = glm::vec<2U, IndexType>;

	using ExtentType = std::dextents<IndexType, 2U>;
	using LayoutType = std::layout_right;
	using MdSpanType = std::mdspan<ValueType, ExtentType, LayoutType>;
	using MappingType = MdSpanType::mapping_type;

private:

	MappingType Mapping;
	std::vector<ValueType, Core::UninitialisedAllocator<ValueType>> Data;

public:

	/**
	 * @brief The total number of regions that are supposed to present in the regionfield. Note that it is likely that not all regions
	 * are on the regionfield. This is purely informational and does not affect the content of the regionfield.
	 */
	ValueType RegionCount {};

	/**
	 * @brief Initialise an empty regionfield matrix.
	 */
	constexpr Regionfield() = default;

	Regionfield(const Regionfield&) = delete;

	constexpr Regionfield(Regionfield&&) noexcept = default;

	Regionfield& operator=(const Regionfield&) = delete;

	constexpr Regionfield& operator=(Regionfield&&) noexcept = default;

	constexpr ~Regionfield() = default;

	[[nodiscard]] constexpr bool operator==(const Regionfield&) const = default;

	/**
	 * @brief Transpose regionfield matrix.
	 *
	 * @return A transpose of the current regionfield matrix.
	 */
	[[nodiscard]] Regionfield transpose() const;

	/**
	 * @brief Resize the regionfield matrix. After this call returns, all existing contents become undefined regardless of whether
	 * reallocation took place. This function provides a performance benefits of reusing existing memory whenever possible.
	 *
	 * @param dim The width and height of the matrix.
	 *
	 * @exception Exception When any component of `dim` is not positive.
	 */
	void resize(DimensionType);

	/**
	 * @brief Get regionfield matrix extent.
	 *
	 * @return Width and height.
	 */
	[[nodiscard]] DimensionType extent() const noexcept;

	/**
	 * @brief Get the linear size of the regionfield matrix.
	 *
	 * @return The total number of region identifiers stored.
	 */
	[[nodiscard]] constexpr IndexType size() const noexcept {
		return this->Data.size();
	}

	/**
	 * @brief Check if the regionfield matrix is empty.
	 * 
	 * @return True if empty.
	 */
	[[nodiscard]] constexpr bool empty() const noexcept {
		return this->Data.empty();
	}

	/**
	 * @brief Get the index mapping of the regionfield.
	 * 
	 * @return Index mapping.
	 */
	[[nodiscard]] constexpr const MappingType& mapping() const noexcept {
		return this->Mapping;
	}

	/**
	 * @brief Get a multi-dimension view on the regionfield matrix.
	 *
	 * @return The mdspan of the regionfield.
	 */
	template<typename Self>
	[[nodiscard]] constexpr auto mdspan(this Self& self) noexcept {
		return std::mdspan(self.Data.data(), self.Mapping);
	}

	/**
	 * @brief Get a 1D view on the regionfield matrix.
	 *
	 * @return The span of the regionfield.
	 */
	template<typename Self>
	[[nodiscard]] constexpr auto span(this Self& self) noexcept {
		return std::span(self.Data);
	}

	/**
	 * @brief Form a 2D view on the regionfield matrix.
	 * 
	 * @return The 2D range of the regionfield.
	 */
	template<typename Self>
	[[nodiscard]] constexpr std::ranges::view auto range2d(this Self& self) noexcept {
		return self.Data | Core::View::Matrix::View2d(self.Mapping.stride(0U));
	}

};

}