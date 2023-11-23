#pragma once

#include <memory>

#include <algorithm>
#include <ranges>
#include <iterator>

#include <type_traits>
#include <cassert>

namespace DisRegRep {

/**
 * @brief Similar to std::vector, but optimised for trivial types.
 * 
 * @tparam T The type of element in this array, which must be trivial.
*/
template<typename T>
requires(std::is_trivial_v<T>)
class TrivialArrayList {
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

	std::unique_ptr<value_type[]> Begin;
	struct {

		pointer Value, Capacity;

	} End { };

	void expand(const size_type curr_size, const size_type curr_cap, size_type new_cap) {
		new_cap = std::max(curr_cap * 2u, new_cap);
		
		auto new_memory = std::make_unique<value_type[]>(new_cap);
		const pointer old_memory_ptr = this->Begin.get(),
			new_memory_ptr = new_memory.get();

		std::ranges::copy_n(old_memory_ptr, curr_size, new_memory_ptr);

		this->Begin = std::move(new_memory);
		this->End = {
			.Value = new_memory_ptr + curr_size,
			.Capacity = new_memory_ptr + curr_cap
		};
	}

	void ensureEnoughToResize(const size_type resized_size) {
		const size_type curr_cap = this->capacity();
		if (resized_size > curr_cap) {
			this->expand(this->size(), curr_cap, resized_size);
		}
	}

	void ensureEnoughToAdd(const size_type additional_size) {
		const size_type curr_size = this->size(),
			curr_cap = this->capacity(),
			new_size = curr_size + additional_size;
		if (new_size > curr_cap) {
			this->expand(curr_size, curr_cap, new_size);
		}
	}

public:

	constexpr TrivialArrayList() noexcept = default;

	TrivialArrayList(const size_type size) {
		this->resize(size);
	}

	TrivialArrayList(const TrivialArrayList&) = delete;

	constexpr TrivialArrayList(TrivialArrayList&& tal) noexcept :
		Begin(std::move(tal.Begin)), End(tal.End) {
		tal.End = { };
	}

	~TrivialArrayList() = default;

	void push_back(const_reference value) {
		this->ensureEnoughToAdd(1u);

		pointer& pushed_to = this->End.Value;
		*pushed_to = value;
		pushed_to++;
	}

	void resize(const size_type size) {
		this->ensureEnoughToResize(size);
		this->End.Value = this->begin() + size;
	}

	constexpr void clear() noexcept {
		this->End.Value = this->begin();
	}

	template<std::ranges::input_range R>
	requires(std::ranges::sized_range<R>)
	void append_range(R&& rg) {
		const size_type append_size = std::ranges::size(rg);
		this->ensureEnoughToAdd(append_size);

		std::ranges::copy(rg, this->End.Value);
		this->End.Value += append_size;
	}

	template<std::ranges::input_range R>
	requires(!std::ranges::sized_range<R>)
	void append_range(R&& rg) {
		//this route is going to be much slower than having a range with known size
		for (const auto& val : rg) {
			this->push_back(val);
		}
	}

	iterator erase(const const_iterator pos) {
		//the standard mandates iterator to be valid, so no need to check if container is empty
		const auto following = const_cast<iterator>(pos);
		std::ranges::copy(pos + 1u, this->cend(), following);
		this->End.Value--;
		return following;
	}

	//CONSIDER: Refactor those standard container functions with a base CRTP.

	constexpr reference operator[](const size_type index) noexcept {
		return const_cast<reference>(const_cast<const TrivialArrayList*>(this)->operator[](index));
	}
	constexpr const_reference operator[](const size_type index) const noexcept {
		assert(index < this->size());
		return this->Begin[index];
	}

	constexpr pointer data() noexcept {
		return const_cast<pointer>(const_cast<const TrivialArrayList*>(this)->data());
	}
	constexpr const_pointer data() const noexcept {
		return this->Begin.get();
	}

	constexpr size_type capacity() const noexcept {
		return std::distance(this->cbegin(), static_cast<const_iterator>(this->End.Capacity));
	}
	constexpr size_type size() const noexcept {
		return std::distance(this->cbegin(), this->cend());
	}
	constexpr bool empty() const noexcept {
		return this->size() != 0u;
	}

	constexpr iterator begin() noexcept {
		return const_cast<iterator>(this->cbegin());
	}
	constexpr const_iterator begin() const noexcept {
		return this->cbegin();
	}
	constexpr const_iterator cbegin() const noexcept {
		return this->Begin.get();
	}

	constexpr iterator end() noexcept {
		return const_cast<iterator>(this->cend());
	}
	constexpr const_iterator end() const noexcept {
		return this->cend();
	}
	constexpr const_iterator cend() const noexcept {
		return this->End.Value;
	}

	constexpr reference front() noexcept {
		return *this->begin();
	}
	constexpr reference back() noexcept {
		return *(this->end() - 1u);
	}

};

}