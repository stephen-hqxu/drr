#pragma once

#include <memory>

#include <concepts>
#include <algorithm>
#include <ranges>
#include <iterator>

#include <cassert>

namespace DisRegRep {

/**
 * @brief A fixed heap array is a array with a fixed heap allocation upon initialisation.
 * Unlike an array list which allows dynamic insertion/deletion of objects,
 * all objects are constructed upon initialisation in an array like this.
 * 
 * @tparam T The type of element in this array.
*/
template<std::default_initializable T>
class FixedHeapArray {
public:

	////////////////////////
	/// STL Container Alias
	////////////////////////

	using value_type = T;
	using size_type = std::size_t;

	using reference = value_type&;
	using const_reference = const value_type&;
	using pointer = value_type*;
	using const_pointer = const value_type*;

	using iterator = pointer;
	using const_iterator = const_pointer;

private:

	std::unique_ptr<value_type[]> Array;
	size_type Count = 0u;

public:

	/**
	 * @brief Create an empty fixed heap array.
	*/
	constexpr FixedHeapArray() noexcept = default;

	/**
	 * @brief Create a fixed heap array with allocation and uninitialised data.
	 *
	 * @param count The number of element to allocate.
	*/
	FixedHeapArray(const size_type count) : Array(std::make_unique_for_overwrite<value_type[]>(count)), Count(count) {

	}

	/**
	 * @brief Create a fixed heap array with filled data.
	 * The iterator overload is useful when size of the range cannot be deduced.
	 * 
	 * @tparam It The iterator type.
	 * @tparam R The input range type.
	 * 
	 * @param first The iterator to the beginning of data.
	 * @param count The number of data to be copied from the iterator.
	 * @param r The input range with data to be copied for initialisation.
	*/
	template<std::input_iterator It>
	FixedHeapArray(const It first, const size_type count) : FixedHeapArray(count) {
		std::copy_n(first, count, this->begin());
	}
	template<class R>
	requires(std::ranges::input_range<R> && std::ranges::sized_range<R>)
	FixedHeapArray(R&& r) : FixedHeapArray(std::ranges::size(r)) {
		std::ranges::copy(r, this->begin());
	}

	constexpr FixedHeapArray(FixedHeapArray&&) noexcept = default;

	constexpr FixedHeapArray& operator=(FixedHeapArray&&) noexcept = default;

	~FixedHeapArray() = default;

	///////////////////////////////
	/// Common Container Function
	///////////////////////////////

	constexpr reference operator[](const size_type index) noexcept {
		return const_cast<reference>(const_cast<const FixedHeapArray*>(this)->operator[](index));
	}
	constexpr const_reference operator[](const size_type index) const noexcept {
		assert(index < this->Count);
		return this->Array[index];
	}

	constexpr pointer data() noexcept {
		return const_cast<pointer>(const_cast<const FixedHeapArray*>(this)->data());
	}
	constexpr const_pointer data() const noexcept {
		return this->Array.get();
	}

	constexpr size_type size() const noexcept {
		return this->Count;
	}
	constexpr bool empty() const noexcept {
		return this->Count != 0u;
	}

	constexpr iterator begin() noexcept {
		return const_cast<iterator>(this->cbegin());
	}
	constexpr const_iterator begin() const noexcept {
		return this->cbegin();
	}
	constexpr const_iterator cbegin() const noexcept {
		return this->Array.get();
	}

	constexpr iterator end() noexcept {
		return const_cast<iterator>(this->cend());
	}
	constexpr const_iterator end() const noexcept {
		return this->cend();
	}
	constexpr const_iterator cend() const noexcept {
		return this->cbegin() + this->Count;
	}

	constexpr reference front() noexcept {
		return *this->begin();
	}
	constexpr reference back() noexcept {
		return *(this->end() - 1u);
	}

};

}