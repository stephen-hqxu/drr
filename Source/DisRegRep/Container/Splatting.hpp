#pragma once

#include "SparseMatrixElement.hpp"

#include <DisRegRep/Type.hpp>
#include <DisRegRep/UninitialisedAllocator.hpp>

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
 * @brief The splatting value matrix (SVM) is a column-major 3D matrix that stores values computed by the convolution used for
 * splatting of region features. The Y and Z axes are used for locating points in a 2D space, and the X axis is used for locating the
 * splatting value for the corresponding region.
 */
namespace DisRegRep::Container::Splatting {

/**
 * @brief Common types used by the SVM implementations.
 */
namespace Type {

using IndexType = std::uint32_t;
using LayoutType = std::layout_left;

template<glm::length_t L>
using DimensionType = glm::vec<L, IndexType>;
using Dimension2Type = DimensionType<2U>; /**< Point coordinate. */
using Dimension3Type = DimensionType<3U>; /**< Region index and point coordinate. */

}

/**
 * @brief A dense SVM is a contiguous 3D matrix of region splatting values.
 *
 * @tparam V Splatting value type.
 */
template<typename>
class BasicDense;

using DenseImportance = BasicDense<DisRegRep::Type::RegionImportance>; /**< Dense region importance. */
using DenseMask = BasicDense<DisRegRep::Type::RegionMask>; /**< Dense region mask. */

/**
 * @brief A sparse SVM is a partial sparse matrix that uses compressed sparse row format on the axis that stores region
 * splatting values (i.e. the X axis), the rest of axes remain dense.
 *
 * @tparam V Splatting value type.
 */
template<typename>
class BasicSparse;

using SparseImportance = BasicSparse<DisRegRep::Type::RegionImportance>; /**< Sparse region importance. */
using SparseMask = BasicSparse<DisRegRep::Type::RegionMask>; /**< Sparse region mask. */

//Compare if the splatting values are equivalent regardless of storage format.
//For sparse matrix, values are assumed to be default initialised if it is not present.
template<typename V>
[[nodiscard]] bool operator==(const BasicDense<V>&, const BasicSparse<V>&);
template<typename V>
[[nodiscard]] bool operator==(const BasicSparse<V>&, const BasicDense<V>&);

#define FRIEND_DENSE_EQ_SPARSE \
	template<typename W> \
	friend bool operator==(const BasicDense<W>&, const BasicSparse<W>&); \
	template<typename W> \
	friend bool operator==(const BasicSparse<W>&, const BasicDense<W>&)

template<typename V>
class BasicDense {
public:

	using ValueType = V;
	using ConstValue = std::add_const_t<ValueType>;
	using IndexType = Type::IndexType;

	using Dimension3Type = Type::Dimension3Type;

	using ExtentType = std::dextents<IndexType, 3U>;
	using LayoutType = Type::LayoutType;
	using MdSpanType = std::mdspan<ValueType, ExtentType, LayoutType>;
	using MappingType = typename MdSpanType::mapping_type;

private:

	using DataContainerType = std::vector<ValueType, UninitialisedAllocator<ValueType>>;

	MappingType Mapping;
	DataContainerType DenseMatrix;

public:

	using SizeType = typename DataContainerType::size_type;

	/**
	 * @brief A proxy of a lvalue reference of per-region splatting values at a coordinate.
	 * 
	 * @param Const True if the values are constant.
	 */
	template<bool Const>
	class ValueProxy {
	public:

		static constexpr bool IsConstant = Const;

		using ProxyElementType = std::conditional_t<IsConstant, ConstValue, ValueType>;
		using ProxyViewType = std::span<ProxyElementType>;
		using ProxyIterator = typename ProxyViewType::iterator;

	private:

		ProxyViewType Span;

	public:

		/**
		 * @brief Initialise a value proxy.
		 *
		 * @tparam R Type of region values.
		 *
		 * @param r A range of values for all regions.
		 */
		template<typename R>
		requires std::is_constructible_v<ProxyViewType, R>
		constexpr ValueProxy(R&& r) noexcept(std::is_nothrow_constructible_v<ProxyViewType, R>) : Span(std::forward<R>(r)) { }

		constexpr ~ValueProxy() = default;

		/**
		 * @brief Get the view of values.
		 *
		 * @return A view of values.
		 */
		[[nodiscard]] constexpr ProxyViewType operator*() const noexcept {
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
		requires std::indirectly_copyable<std::ranges::const_iterator_t<Value>, ProxyIterator>
		constexpr ValueProxy& operator=(Value&& value)
		requires(!IsConstant)
		{
			using std::copy, std::execution::unseq,
				std::ranges::cbegin, std::ranges::cend;
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

	[[nodiscard]] bool operator==(const BasicDense&) const;

	FRIEND_DENSE_EQ_SPARSE;

	/**
	 * @brief Get the size of the dense matrix in bytes.
	 *
	 * @return Size in bytes.
	 */
	[[nodiscard]] SizeType sizeByte() const noexcept;

	/**
	 * @brief Resize the current dense matrix. All existing contents become undefined.
	 *
	 * @param dim Provide region count, width and height of the dense matrix.
	 */
	void resize(Dimension3Type);

	/**
	 * @brief This function must be called to reset the internal state of the matrix before commencing new computations. For dense
	 * matrix, it is currently a no-op as there is no internal states involved; keeping it to ensure API consistency with the sparse
	 * implementation.
	 */
	constexpr void clear() const noexcept { }

	/**
	 * @brief Get a range to the dense matrix.
	 * 
	 * @return A range to the dense matrix.
	 */
	template<typename Self>
	[[nodiscard]] constexpr auto range(this Self& self) noexcept {
		using std::views::chunk, std::views::transform;
		using ProxyType = ValueProxy<std::is_const_v<Self>>;

		return self.DenseMatrix
			 | chunk(self.Mapping.stride(1U))
			 | transform([](auto region_val) static constexpr noexcept { return ProxyType(std::move(region_val)); });
	}

};

template<typename V>
class BasicSparse {
public:

	using ValueType = V;
	using ElementType = SparseMatrixElement::Basic<ValueType>;
	using ConstElement = std::add_const_t<ElementType>;
	using OffsetType = std::uint32_t;
	using ConstOffset = std::add_const_t<OffsetType>;
	using IndexType = Type::IndexType;

	using Dimension3Type = Type::Dimension3Type;

	using OffsetExtentType = std::dextents<IndexType, 2U>;
	using OffsetLayoutType = Type::LayoutType;
	using OffsetMdSpanType = std::mdspan<OffsetType, OffsetExtentType, OffsetLayoutType>;
	using OffsetMappingType = OffsetMdSpanType::mapping_type;

private:

	using OffsetContainerType = std::vector<OffsetType, UninitialisedAllocator<OffsetType>>;
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
	 * @brief A proxy of per-region splatting values at a coordinate.
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
		constexpr ValueProxy& operator=(Value&& value)
		requires(!IsConstant)
		{
			*this = std::forward<Value>(value) | SparseMatrixElement::ToSparse;
			return *this;
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
		constexpr ValueProxy& operator=(Value&& value)
		requires(!IsConstant)
		{
			auto& [_, next] = this->PairwiseOffset;
			this->ElementContainer->append_range(std::forward<Value>(value));
			next = this->ElementContainer->size();
			return *this;
		}

	};

	constexpr BasicSparse() noexcept = default;

	BasicSparse(const BasicSparse&) = delete;

	constexpr BasicSparse(BasicSparse&&) noexcept = default;

	BasicSparse& operator=(const BasicSparse&) = delete;

	constexpr BasicSparse& operator=(BasicSparse&&) noexcept = default;

	constexpr ~BasicSparse() = default;

	[[nodiscard]] bool operator==(const BasicSparse&) const;

	FRIEND_DENSE_EQ_SPARSE;

	/**
	 * @brief Sort the splatting values of each element in the sparse matrix, in ascending order of region identifier.
	 *
	 * @link BasicSparse::isSorted
	 */
	void sort();

	/**
	 * @brief Check if the splatting values of each element in the sparse matrix is sorted.
	 *
	 * @link BasicSparse::sort
	 *
	 * @return True if all sorted.
	 */
	[[nodiscard]] bool isSorted() const;

	/**
	 * @brief Get the size of the sparse matrix in bytes.
	 *
	 * @return Size in bytes.
	 */
	[[nodiscard]] SizeType sizeByte() const noexcept;

	/**
	 * @brief Resize the current sparse matrix. All existing contents become undefined.
	 *
	 * @param dim Provide width and height of the sparse matrix. The region count is sparsely populated, but should still be provided
	 * as is similar to that of the dense matrix to keep API consistency.
	 */
	void resize(Dimension3Type);

	/**
	 * @brief This function must be called to reset the internal state of the matrix and clear old values before commencing new
	 * computations.
	 */
	void clear();

	/**
	 * @brief Get a range to the sparse matrix.
	 * 
	 * @return A range to the sparse matrix.
	 */
	template<typename Self>
	[[nodiscard]] constexpr auto range(this Self& self) noexcept {
		using std::views::pairwise, std::views::transform;
		using ProxyType = ValueProxy<std::is_const_v<Self>>;

		//Not using pairwise_transform since I need to pass the original tuple to the proxy.
		return self.Offset
			| pairwise
			| transform([&elem = self.SparseMatrix](auto pairwise_offset) constexpr noexcept {
				return ProxyType(std::move(pairwise_offset), elem);
			});
	}

};

#undef FRIEND_DENSE_EQ_SPARSE

}