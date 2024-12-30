#pragma once

#include <DisRegRep/Core/Arithmetic.hpp>
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
 * @brief Regionfield is a function that tells which region a given point belongs to. In this project regionfield is constructed as a
 * 2D matrix of region identifiers. The storage uses column-major order.
 */
class Regionfield {
public:

	using ValueType = Core::Type::RegionIdentifier;
	using ConstValue = std::add_const_t<ValueType>;
	using IndexType = std::uint_fast32_t;
	using DimensionType = glm::vec<2U, IndexType>;

	using ExtentType = std::dextents<IndexType, 2U>;
	using LayoutType = std::layout_left;
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
	 * @brief Get the linear size of the regionfield matrix.
	 *
	 * @return The total number of region identifiers stored.
	 */
	[[nodiscard]] constexpr IndexType size() const noexcept {
		return this->Data.size();
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
		return std::mdspan(self.Data.data(), self.mapping());
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
		return self.Data | Core::Arithmetic::View2d(self.mapping().stride(1U));
	}

};

}