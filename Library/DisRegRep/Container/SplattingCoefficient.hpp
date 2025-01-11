#pragma once

#include "SparseMatrixElement.hpp"

#include <DisRegRep/Core/Arithmetic.hpp>
#include <DisRegRep/Core/Type.hpp>
#include <DisRegRep/Core/UninitialisedAllocator.hpp>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <mdspan>
#include <span>
#include <tuple>
#include <vector>

#include <algorithm>
#include <execution>
#include <iterator>
#include <ranges>

#include <memory>
#include <utility>

#include <type_traits>

#include <cstdint>

/**
 * @brief Splatting coefficients are values computed by the convolution used for splatting of region features. In this project there
 * are mainly two kinds of splatting coefficients, being region importance and region mask. The splatting coefficients are stored in a
 * 3D column-major matrix, namely the splatting coefficient matrix (SCM). The Y and Z axes are used for locating points in a 2D space,
 * and the X axis is used for locating the splatting coefficients for the corresponding region.
 */
namespace DisRegRep::Container::SplattingCoefficient {

/**
 * @brief Common types used by the SCM implementations.
 */
namespace Type {

using IndexType = std::uint_fast32_t;
using LayoutType = std::layout_left;

template<glm::length_t L>
using DimensionType = glm::vec<L, IndexType>;
using Dimension2Type = DimensionType<2U>; /**< Point coordinate. */
using Dimension3Type = DimensionType<3U>; /**< Region index and point coordinate. */

}

/**
 * @brief A dense SCM is a contiguous 3D matrix of region splatting coefficients.
 *
 * @tparam V Splatting coefficient value type.
 */
template<typename>
class BasicDense;

using DenseImportance = BasicDense<Core::Type::RegionImportance>; /**< Dense region importance. */
using DenseMask = BasicDense<Core::Type::RegionMask>; /**< Dense region mask. */

/**
 * @brief A sparse SCM is a partial sparse matrix that uses compressed sparse row format on the axis that stores region
 * splatting coefficients (i.e. the X axis), the rest of axes remain dense.
 *
 * @tparam V Splatting coefficient value type.
 */
template<typename>
class BasicSparse;

using SparseImportance = BasicSparse<Core::Type::RegionImportance>; /**< Sparse region importance. */
using SparseMask = BasicSparse<Core::Type::RegionMask>; /**< Sparse region mask. */

template<typename V>
class BasicDense {
public:

	using ValueType = V;
	using ElementType = ValueType;
	using ConstElement = std::add_const_t<ElementType>;
	using IndexType = Type::IndexType;

	using Dimension3Type = Type::Dimension3Type;

	using ExtentType = std::dextents<IndexType, 3U>;
	using LayoutType = Type::LayoutType;
	using MdSpanType = std::mdspan<ElementType, ExtentType, LayoutType>;
	using MappingType = typename MdSpanType::mapping_type;

private:

	using DataContainerType = std::vector<ElementType, Core::UninitialisedAllocator<ElementType>>;

	MappingType Mapping;
	DataContainerType DenseMatrix;

public:

	using SizeType = typename DataContainerType::size_type;

	/**
	 * @brief A proxy that acts like a lvalue reference to values along the X-axis (i.e. the per-region splatting coefficient at fixed
	 * coordinate) of the dense matrix.
	 *
	 * @param Const True if the values are constant.
	 */
	template<bool Const>
	class ValueProxy {
	public:

		static constexpr bool IsConstant = Const;

		using ProxyElementType = std::conditional_t<IsConstant, ConstElement, ElementType>;
		using ProxyElementViewType = std::span<ProxyElementType>;
		using ProxyElementIterator = typename ProxyElementViewType::iterator;

	private:

		ProxyElementViewType Span;

	public:

		/**
		 * @brief Initialise a value proxy.
		 *
		 * @tparam R Type of range of values.
		 *
		 * @param r A range of values.
		 */
		template<typename R>
		requires std::is_constructible_v<ProxyElementViewType, R>
		constexpr ValueProxy(R&& r) noexcept(std::is_nothrow_constructible_v<ProxyElementViewType, R>) : Span(std::forward<R>(r)) { }

		constexpr ~ValueProxy() = default;

		/**
		 * @brief Get the view of values.
		 *
		 * @return A view of values.
		 */
		[[nodiscard]] constexpr ProxyElementViewType operator*() const noexcept {
			return this->Span;
		}

		/**
		 * @brief Copy a given range of dense matrix to the view in this proxy.
		 *
		 * @tparam Value Type of range to be copied.
		 *
		 * @param value Values to be copied.
		 *
		 * @return Self.
		 */
		template<std::ranges::forward_range Value>
		requires std::ranges::common_range<Value>
			  && std::indirectly_copyable<std::ranges::const_iterator_t<Value>, ProxyElementIterator>
				 const ValueProxy& operator=(Value&& value) const
				 requires(!IsConstant)
		{
			using std::copy, std::execution::unseq, std::ranges::cbegin, std::ranges::cend;
			copy(unseq, cbegin(value), cend(value), this->Span.begin());
			return *this;
		}

	};

	constexpr BasicDense() = default;

	BasicDense(const BasicDense&) = delete;

	constexpr BasicDense(BasicDense&&) noexcept = default;

	BasicDense& operator=(const BasicDense&) = delete;

	constexpr BasicDense& operator=(BasicDense&&) noexcept = default;

	constexpr ~BasicDense() = default;

	/**
	 * @brief Get the index mapping of the dense matrix.
	 *
	 * @return Index mapping.
	 */
	[[nodiscard]] constexpr const MappingType& mapping() const noexcept {
		return this->Mapping;
	}

	/**
	 * @brief Get the linear size of the dense matrix.
	 *
	 * @return The total number of splatting coefficient stored.
	 */
	[[nodiscard]] constexpr IndexType size() const noexcept {
		return this->DenseMatrix.size();
	}

	/**
	 * @brief Check if the dense matrix is empty.
	 *
	 * @return True if empty.
	 */
	[[nodiscard]] constexpr bool empty() const noexcept {
		return this->DenseMatrix.empty();
	}

	/**
	 * @brief Get the size of the dense matrix in bytes.
	 *
	 * @return Size in bytes.
	 */
	[[nodiscard]] SizeType sizeByte() const noexcept;

	/**
	 * @brief Resize the current dense matrix. All existing contents become undefined and the internal state of the matrix is reset,
	 * thus suitable for commencing new computations.
	 *
	 * @param dim Provide region count, width and height of the dense matrix.
	 */
	void resize(Dimension3Type);

	/**
	 * @brief Get a range to the dense matrix.
	 * 
	 * @return A range to the dense matrix.
	 */
	template<typename Self>
	[[nodiscard]] constexpr std::ranges::view auto range(this Self& self) noexcept {
		using std::views::transform;
		using ProxyType = ValueProxy<std::is_const_v<Self>>;

		return self.DenseMatrix
			 | Core::Arithmetic::View2d(self.Mapping.stride(1U))
			 | transform([](auto region_val) static constexpr noexcept { return ProxyType(std::move(region_val)); });
	}

	/**
	 * @brief Get a transposed 2D range to the dense matrix.
	 *
	 * @return A transposed 2D range to the dense matrix.
	 */
	[[nodiscard]] constexpr std::ranges::view auto rangeTransposed2d() const noexcept {
		return this->range() | Core::Arithmetic::ViewTransposed2d(this->Mapping.extents().extent(1U));
	}

};

template<typename V>
class BasicSparse {
public:

	using ValueType = V;
	using ElementType = SparseMatrixElement::Basic<ValueType>;
	using ConstElement = std::add_const_t<ElementType>;
	using OffsetType = std::uint_least32_t;
	using ConstOffset = std::add_const_t<OffsetType>;
	using IndexType = Type::IndexType;

	using Dimension3Type = Type::Dimension3Type;

	using OffsetExtentType = std::dextents<IndexType, 2U>;
	using OffsetLayoutType = Type::LayoutType;
	using OffsetMdSpanType = std::mdspan<OffsetType, OffsetExtentType, OffsetLayoutType>;
	using OffsetMappingType = OffsetMdSpanType::mapping_type;

private:

	using OffsetContainerType = std::vector<OffsetType>;
	using ElementContainerType = std::vector<ElementType>;
	using ConstElementContainerType = std::add_const_t<ElementContainerType>;

	OffsetMappingType OffsetMapping;
	OffsetContainerType Offset;
	ElementContainerType SparseMatrix;

	//Linear size of the dense offset matrix.
	[[nodiscard]] constexpr IndexType sizeOffset() const noexcept {
		//The final offset is the total number of elements in the sparse matrix,
		//	so we can later use pairwise view to compute the span of every element.
		return this->OffsetMapping.required_span_size() + 1U;
	}

public:

	using SizeType = std::common_type_t<OffsetContainerType::size_type, typename ElementContainerType::size_type>;

	/**
	 * @brief A proxy of values along the X-axis (i.e. the per-region splatting coefficient at fixed coordinate) of the sparse matrix.
	 *
	 * @tparam Const True if the values are constant.
	 */
	template<bool Const>
	class ValueProxy {
	public:

		static constexpr bool IsConstant = Const;

		using ProxyElementType = std::conditional_t<IsConstant, ConstElement, ElementType>;
		using ProxyElementViewType = std::span<ProxyElementType>;

		using ProxyElementContainerType = std::conditional_t<IsConstant, ConstElementContainerType, ElementContainerType>;
		using ProxyElementContainerPointer = std::add_pointer_t<ProxyElementContainerType>;
		using ProxyElementContainerReference = std::add_lvalue_reference_t<ProxyElementContainerType>;

		using ProxyOffsetType = std::conditional_t<IsConstant, ConstOffset, OffsetType>;
		using ProxyOffsetReference = std::add_lvalue_reference_t<ProxyOffsetType>;
		using ProxyPairwiseOffsetType = std::tuple<ProxyOffsetReference, ProxyOffsetReference>;

	private:

		ProxyPairwiseOffsetType PairwiseOffset;
		ProxyElementContainerPointer ElementContainer;

	public:

		/**
		 * @brief Initialise a value proxy.
		 *
		 * @param pairwise_offset The offset of the **next** element in the contiguous memory order.
		 * @param sparse_matrix The sparse matrix container.
		 */
		constexpr ValueProxy(ProxyPairwiseOffsetType pairwise_offset, ProxyElementContainerReference sparse_matrix) noexcept :
			PairwiseOffset(std::move(pairwise_offset)), ElementContainer(std::addressof(sparse_matrix)) { }

		constexpr ~ValueProxy() = default;

		/**
		 * @brief Get the view of values.
		 *
		 * @return View of values.
		 */
		[[nodiscard]] constexpr ProxyElementViewType operator*() const noexcept {
			const auto [prev, next] = this->PairwiseOffset;
			return { this->ElementContainer->begin() + prev, next - prev };
		}

		/**
		 * @brief Append a range of sparse matrix to the view in this proxy.
		 *
		 * @tparam Value Type of range value.
		 *
		 * @param value Values to be appended.
		 *
		 * @return Self.
		 */
		template<std::ranges::input_range Value>
		requires std::is_same_v<std::ranges::range_value_t<Value>, ElementType>
		constexpr const ValueProxy& operator=(Value&& value) const
		requires(!IsConstant)
		{
			auto& [_, next] = this->PairwiseOffset;
			this->ElementContainer->append_range(std::forward<Value>(value));
			next = this->ElementContainer->size();
			return *this;
		}

		/**
		 * @brief Append a range of dense matrix to the view in this proxy.
		 *
		 * @tparam Value Type of range value.
		 *
		 * @param value Values to be appended.
		 *
		 * @return Self.
		 */
		template<std::ranges::input_range Value>
		requires std::is_convertible_v<std::ranges::range_value_t<Value>, ValueType>
		constexpr const ValueProxy& operator=(Value&& value) const
		requires(!IsConstant)
		{
			*this = std::forward<Value>(value) | SparseMatrixElement::ToSparse;
			return *this;
		}

	};

	constexpr BasicSparse() noexcept = default;

	BasicSparse(const BasicSparse&) = delete;

	constexpr BasicSparse(BasicSparse&&) noexcept = default;

	BasicSparse& operator=(const BasicSparse&) = delete;

	constexpr BasicSparse& operator=(BasicSparse&&) noexcept = default;

	constexpr ~BasicSparse() = default;

	/**
	 * @brief Sort the values of each element in the sparse matrix, in ascending order of region identifier.
	 *
	 * @link BasicSparse::isSorted
	 */
	void sort();

	/**
	 * @brief Check if the values of each element in the sparse matrix is sorted.
	 *
	 * @link BasicSparse::sort
	 *
	 * @return True if all sorted.
	 */
	[[nodiscard]] bool isSorted() const;

	/**
	 * @brief Get the index mapping of the sparse matrix.
	 *
	 * @note Since sparse matrix is only *partially* sparse, i.e. the per-region splatting coefficient axis is sparse, the mapping only
	 * maps the desne axes.
	 *
	 * @return Index mapping.
	 */
	[[nodiscard]] constexpr const OffsetMappingType& mapping() const noexcept {
		return this->OffsetMapping;
	}

	/**
	 * @brief Get the linear size of the sparse matrix.
	 *
	 * @return The total number of splatting coefficient stored.
	 */
	[[nodiscard]] constexpr IndexType size() const noexcept {
		return this->SparseMatrix.size();
	}

	/**
	 * @brief Check if the sparse matrix is empty.
	 *
	 * @return True if empty.
	 */
	[[nodiscard]] constexpr bool empty() const noexcept {
		return this->SparseMatrix.empty();
	}

	/**
	 * @brief Get the size of the sparse matrix in bytes.
	 *
	 * @return Size in bytes.
	 */
	[[nodiscard]] SizeType sizeByte() const noexcept;

	/**
	 * @brief Resize the current sparse matrix. All existing contents become undefined and the internal state of the matrix is reset,
	 * thus suitable for commencing new computations.
	 *
	 * @param dim Provide width and height of the sparse matrix. The region count is sparsely populated, but should still be provided
	 * as is similar to that of the dense matrix to keep API consistency.
	 */
	void resize(Dimension3Type);

	/**
	 * @brief Get a range to the sparse matrix.
	 * 
	 * @return A range to the sparse matrix.
	 */
	template<typename Self>
	[[nodiscard]] constexpr std::ranges::view auto range(this Self& self) noexcept {
		using std::views::pairwise, std::views::transform;
		using ProxyType = ValueProxy<std::is_const_v<Self>>;

		//Not using pairwise_transform since I need to pass the original tuple to the proxy.
		return self.Offset
			| pairwise
			| transform([&elem = self.SparseMatrix](auto pairwise_offset) constexpr noexcept {
				return ProxyType(std::move(pairwise_offset), elem);
			});
	}

	/**
	 * @brief Get a transposed 2D range to the sparse matrix.
	 *
	 * @return A transposed 2D range to the sparse matrix.
	 */
	[[nodiscard]] constexpr std::ranges::view auto rangeTransposed2d() const noexcept {
		return this->range() | Core::Arithmetic::ViewTransposed2d(this->OffsetMapping.stride(1U));
	}

};

/**
 * `Mat` is a specialisation of `BasicDense`.
 */
template<typename Mat>
concept IsDense = std::is_same_v<Mat, BasicDense<typename Mat::ValueType>>;

/**
 * `Mat` is a specialisation of `BasicSparse`.
 */
template<typename Mat>
concept IsSparse = std::is_same_v<Mat, BasicSparse<typename Mat::ValueType>>;

/**
 * `Mat` is a splatting coefficient matrix;
 */
template<typename Mat>
concept Is = IsDense<Mat> || IsSparse<Mat>;

}