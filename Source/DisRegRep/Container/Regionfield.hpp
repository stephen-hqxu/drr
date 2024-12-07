#pragma once

#include <DisRegRep/Type.hpp>

#include <glm/vec2.hpp>
#include <mdspan/mdspan.hpp>

#include <memory>
#include <span>

#include <type_traits>

#include <cstdint>

namespace DisRegRep::Container {

/**
 * @brief Regionfield is a function that tells which region a given point belongs to. In this project regionfield is constructed as a
 * 2D matrix of region identifiers. The storage uses column-major order.
 */
class Regionfield {
public:

	using ValueType = Type::RegionIdentifier;
	using ConstValue = std::add_const_t<ValueType>;
	using IndexType = std::uint32_t;
	using DimensionType = glm::vec<2U, IndexType>;

	using SpanType = std::span<ValueType>;
	using ConstSpanType = std::span<ConstValue>;

	using ExtentType = Kokkos::dextents<IndexType, 2U>;
	using LayoutType = Kokkos::layout_left;
	using MdSpanType = Kokkos::mdspan<ValueType, ExtentType, LayoutType>;
	using ConstMdSpanType = Kokkos::mdspan<ConstValue, ExtentType, LayoutType>;
	using MappingType = MdSpanType::mapping_type;

private:

	MappingType Mapping;
	std::unique_ptr<ValueType[]> Data;

	ValueType RegionCount {};

public:

	/**
	 * @brief Initialise an empty regionfield matrix.
	 */
	constexpr Regionfield() = default;

	/**
	 * @brief Allocate a regionfield matrix with uninitialised data.
	 *
	 * @param dim The width and height of the matrix.
	 * @param region_count The total number of regions that are supposed to present in the regionfield. Note that it is likely that not
	 * all regions are on the regionfield.
	 */
	Regionfield(DimensionType, ValueType);

	Regionfield(const Regionfield&) = delete;

	constexpr Regionfield(Regionfield&&) noexcept = default;

	Regionfield& operator=(const Regionfield&) = delete;

	constexpr Regionfield& operator=(Regionfield&&) noexcept = default;

	constexpr ~Regionfield() = default;

	/**
	 * @brief Get the linear size of the regionfield matrix.
	 *
	 * @return The total number of region identifiers stored.
	 */
	[[nodiscard]] constexpr IndexType size() const noexcept {
		return this->Mapping.required_span_size();
	}

	/**
	 * @brief Get the total number of regions.
	 * 
	 * @return Total number of regions.
	 */
	[[nodiscard]] constexpr ValueType regionCount() const noexcept {
		return this->RegionCount;
	}

	/**
	 * @brief Get a multi-dimension view on the regionfield matrix.
	 *
	 * @return The mdspan of the regionfield.
	 */
	[[nodiscard]] constexpr MdSpanType mdspan() noexcept {
		return { this->Data.get(), this->Mapping };
	}
	[[nodiscard]] constexpr ConstMdSpanType mdspan() const noexcept {
		return { this->Data.get(), this->Mapping };
	}

	/**
	 * @brief Get a 1D view on the regionfield matrix.
	 *
	 * @return The span of the regionfield.
	 */
	[[nodiscard]] constexpr SpanType span() noexcept {
		return { this->Data.get(), this->size() };
	}
	[[nodiscard]] constexpr ConstSpanType span() const noexcept {
		return { this->Data.get(), this->size() };
	}

};

}