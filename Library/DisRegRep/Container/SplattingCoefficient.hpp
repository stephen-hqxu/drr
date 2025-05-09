#pragma once

#include "SparseMatrixElement.hpp"

#include <DisRegRep/Core/View/Matrix.hpp>
#include <DisRegRep/Core/View/Trait.hpp>
#include <DisRegRep/Core/Type.hpp>
#include <DisRegRep/Core/UninitialisedAllocator.hpp>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <mdspan>
#include <span>
#include <tuple>
#include <vector>

#include <algorithm>
#include <iterator>
#include <ranges>

#include <memory>
#include <utility>

#include <type_traits>

#include <cstdint>

/**
 * @brief Splatting coefficients are values computed for the splatting of region features, and in this project, there are two kinds of
 * splatting coefficients: region importance and region mask. The splatting coefficients are stored in a 3D matrix $w_{W,H,N}$ (i.e.
 * the splatting coefficient matrix (SCM)), where $W$, $H$ and $N$ denote the width, height and region count of the matrix,
 * respectively. The expression $w[r,c,s]$ is the splatting coefficient at row $r$, column $c$ and region $s$, with the region axis
 * having a stride of one.
 */
namespace DisRegRep::Container::SplattingCoefficient {

/**
 * @brief Common types used by the SCM implementations.
 */
namespace Type {

using IndexType = std::uint_fast32_t;
using LayoutType = std::layout_right;

template<glm::length_t L>
using DimensionType = glm::vec<L, IndexType>;
using Dimension2Type = DimensionType<2U>; /**< Point coordinate. */
using Dimension3Type = DimensionType<3U>; /**< Point coordinate and region identifier. */

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
 * @brief A sparse SCM is a partial sparse matrix that uses compressed sparse format on the region axis (i.e. the Z axis), the rest of
 * axes remain dense.
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

	using Dimension2Type = Type::Dimension2Type;
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
	 * @brief A proxy that acts like a lvalue reference to values along the Z axis of the dense matrix.
	 *
	 * @param Const True if the values are constant.
	 * @param ElemView Region axis view.
	 */
	template<bool Const, std::ranges::view ElemView>
	requires std::is_same_v<std::ranges::range_value_t<ElemView>, ElementType>
	class ValueProxy {
	public:

		static constexpr bool IsConstant = Const;

		using ProxyElementViewType = ElemView;
		using ProxyElementViewIterator = std::conditional_t<IsConstant,
			std::ranges::const_iterator_t<ProxyElementViewType>,
			std::ranges::iterator_t<ProxyElementViewType>
		>;

	private:

		ProxyElementViewType Element;

	public:

		/**
		 * @brief Initialise a value proxy.
		 *
		 * @tparam DeducingConst Constness of this value proxy for use by CTAD.
		 * @tparam R Type of range of values.
		 *
		 * @param r A range of values.
		 */
		template<bool DeducingConst, std::ranges::viewable_range R>
		//Clang will  ^^^^^^^^^^^^^ crash if the class argument is captured directly here; need to write a deduction guide.
		constexpr ValueProxy(std::bool_constant<DeducingConst>, R&& r) noexcept(Core::View::Trait::IsNothrowViewable<R>) :
			Element(std::forward<R>(r) | std::views::all) { }

		/**
		 * @brief Get the view of values.
		 *
		 * @return A view of values.
		 */
		[[nodiscard]] constexpr const ProxyElementViewType& operator*() const & noexcept {
			return this->Element;
		}
		[[nodiscard]] constexpr ProxyElementViewType operator*() && noexcept(
			std::is_nothrow_move_constructible_v<ProxyElementViewType>) {
			return std::move(this->Element);
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
		template<std::ranges::input_range Value>
		requires std::indirectly_copyable<std::ranges::const_iterator_t<Value>, ProxyElementViewIterator>
		const ValueProxy& operator=(Value&& value) const
		requires(!IsConstant)
		{
			std::ranges::copy(std::forward<Value>(value), std::ranges::begin(this->Element));
			return *this;
		}

	};
	template<bool Const, typename R>
	ValueProxy(std::bool_constant<Const>, R&&) -> ValueProxy<Const, std::views::all_t<R>>;

	constexpr BasicDense() = default;

	BasicDense(const BasicDense&) = delete;

	constexpr BasicDense(BasicDense&&) noexcept = default;

	BasicDense& operator=(const BasicDense&) = delete;

	constexpr BasicDense& operator=(BasicDense&&) noexcept = default;

	constexpr ~BasicDense() = default;

	/**
	 * @brief Get the dense matrix extent.
	 *
	 * @return Dense matrix extent.
	 */
	[[nodiscard]] Dimension3Type extent() const noexcept;

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
	 * @brief Resize the current dense matrix. All existing contents become undefined, and the internal state of the matrix is reset,
	 * thus suitable for commencing new computations.
	 *
	 * @param dim Provide width, height and region count of the dense matrix.
	 */
	void resize(Dimension3Type);

	/**
	 * @brief Get a range to the dense matrix.
	 *
	 * @return A range to the dense matrix.
	 */
	template<typename Self>
	[[nodiscard]] constexpr std::ranges::view auto range(this Self& self) noexcept {
		using std::views::transform, std::bool_constant;

		return self.DenseMatrix
			| Core::View::Matrix::NewAxisLeft(self.Mapping.stride(1U))
			| transform([](auto region_val) static constexpr noexcept {
				return ValueProxy(bool_constant<std::is_const_v<Self>> {}, std::move(region_val));
			});
	}

	/**
	 * @brief Get a 2D range to the dense matrix.
	 *
	 * @return A 2D range to the dense matrix.
	 */
	[[nodiscard]] constexpr std::ranges::view auto range2d(this auto& self) noexcept {
		return self.range() | Core::View::Matrix::NewAxisLeft(self.Mapping.extents().extent(1U));
	}

	/**
	 * @brief Get a transposed 2D range to the dense matrix.
	 *
	 * @return A transposed 2D range to the dense matrix.
	 */
	[[nodiscard]] constexpr std::ranges::view auto rangeTransposed2d() const noexcept {
		return this->range() | Core::View::Matrix::NewAxisRight(this->Mapping.extents().extent(1U));
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

	using Dimension2Type = Type::Dimension2Type;
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
	 * @brief A proxy of values along the Z axis of the sparse matrix.
	 *
	 * @tparam Const True if the values are constant.
	 */
	template<bool Const>
	class ValueProxy {
	public:

		static constexpr bool IsConstant = Const;

		using ProxyElementType = std::conditional_t<IsConstant, ConstElement, ElementType>;
		using ProxyElementViewType = std::span<ProxyElementType>;
		using ProxyElementViewIterator = typename ProxyElementViewType::iterator;

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
		requires std::ranges::viewable_range<Value>
			  && std::is_convertible_v<std::ranges::range_value_t<Value>, ValueType>
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
	 * @brief Get the sparse matrix extent.
	 *
	 * @note Since the sparse matrix is only *partially* sparse on the Z axis, the mapping only maps the dense axes.
	 *
	 * @return Sparse matrix extent.
	 */
	[[nodiscard]] Dimension2Type extent() const noexcept;

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
	 * @brief Resize the current sparse matrix. All existing contents become undefined, and the internal state of the matrix is reset,
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
	 * @brief Get a 2D range to the sparse matrix.
	 *
	 * @return A 2D range to the sparse matrix.
	 */
	[[nodiscard]] constexpr std::ranges::view auto range2d(this auto& self) noexcept {
		return self.range() | Core::View::Matrix::NewAxisLeft(self.OffsetMapping.stride(0U));
	}

	/**
	 * @brief Get a transposed 2D range to the sparse matrix.
	 *
	 * @return A transposed 2D range to the sparse matrix.
	 */
	[[nodiscard]] constexpr std::ranges::view auto rangeTransposed2d() const noexcept {
		return this->range() | Core::View::Matrix::NewAxisRight(this->OffsetMapping.stride(0U));
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