#pragma once

#include "CommonExpositionOnly.hpp"
#include "RangeAdaptorClosure.hpp"
#include "Trait.hpp"

#include <iterator>
#include <ranges>

#include <utility>

#include <concepts>
#include <type_traits>

#include <version>

#if __cpp_lib_ranges_to_input >= 2025'02L
#define DRR_CORE_VIEW_HAS_STD_RANGES_TO_INPUT
#endif

namespace DisRegRep::Core::View {

#ifndef DRR_CORE_VIEW_HAS_STD_RANGES_TO_INPUT
/**
 * @brief Reference implementation of C++26 std::ranges::to_input_view.
 *
 * @ref https://isocpp.org/files/papers/P3137R3.html
 *
 * @tparam V Range to be forced to become a @link std::ranges::input_range regardless its previous category.
 */
template<std::ranges::input_range V>
requires std::ranges::view<V>
class ToInputView : public std::ranges::view_interface<ToInputView<V>> {
public:

	using ViewType = V;

	using ViewIterator = std::ranges::iterator_t<ViewType>;

private:

	ViewType Base;

	template<bool Const>
	class Iterator {
	private:

		friend ToInputView;

		using Base = CommonExpositionOnly::MaybeConst<Const, ViewType>;
		using BaseRValueReference = std::ranges::range_rvalue_reference_t<Base>;
		using BaseIterator = std::ranges::iterator_t<Base>;
		using BaseSentinel = std::ranges::sentinel_t<Base>;

		BaseIterator Current;

		explicit constexpr Iterator(BaseIterator current) noexcept(std::is_nothrow_move_constructible_v<BaseIterator>) :
			Current(std::move(current)) { }

	public:

		using difference_type = std::ranges::range_difference_t<Base>;
		using value_type = std::ranges::range_value_t<Base>;
		using iterator_concept = std::input_iterator_tag;

		Iterator() noexcept(std::is_nothrow_default_constructible_v<BaseIterator>)
		requires std::default_initializable<BaseIterator>
		= default;

		constexpr Iterator(Iterator<!Const> it) noexcept(std::is_nothrow_move_constructible_v<BaseIterator>)
		requires Const && std::convertible_to<ViewIterator, BaseIterator>
			: Current(std::move(it.Current)) { }

		[[nodiscard]] constexpr BaseIterator base() && noexcept(std::is_nothrow_move_constructible_v<BaseIterator>) {
			return std::move(this->Current);
		}

		[[nodiscard]] constexpr const BaseIterator& base() const& noexcept {
			return this->Current;
		}

		[[nodiscard]] constexpr decltype(auto) operator*() const noexcept(noexcept(*this->Current)) {
			return *this->Current;
		}

		constexpr Iterator& operator++() noexcept(noexcept(++this->Current)) {
			++this->Current;
			return *this;
		}

		constexpr void operator++(int) noexcept(noexcept(++*this)) {
			++*this;
		}

		[[nodiscard]] friend constexpr bool operator==(const Iterator& x, const BaseSentinel& y) noexcept(noexcept(x.Current == y)) {
			return x.Current == y;
		}

		[[nodiscard]] friend constexpr difference_type operator-(const BaseSentinel& y, const Iterator& x)
			noexcept(noexcept(y - x.Current))
		requires std::sized_sentinel_for<BaseSentinel, BaseIterator>
		{
			return y - x.Current;
		}

		[[nodiscard]] friend constexpr difference_type operator-(const Iterator& x, const BaseSentinel& y)
			noexcept(noexcept(x.Current - y))
		requires std::sized_sentinel_for<BaseSentinel, BaseIterator>
		{
			return x.Current - y;
		}

		[[nodiscard]] friend constexpr BaseRValueReference iter_move(const Iterator& it)
			noexcept(noexcept(std::ranges::iter_move(it.Current))) {
			return std::ranges::iter_move(it.Current);
		}

		friend constexpr void iter_swap(const Iterator& x, const Iterator& y)
			noexcept(noexcept(std::ranges::iter_swap(x.Current, y.Current)))
		requires std::indirectly_swappable<BaseIterator>
		{
			std::ranges::iter_swap(x.Current, y.Current);
		}

	};

public:

	ToInputView() noexcept(std::is_nothrow_default_constructible_v<ViewType>)
	requires std::default_initializable<ViewType>
	= default;

	explicit constexpr ToInputView(ViewType base) noexcept(std::is_nothrow_move_constructible_v<ViewType>) : Base(std::move(base)) { }

	[[nodiscard]] constexpr ViewType base() const& noexcept(std::is_nothrow_copy_constructible_v<ViewType>)
	requires std::copy_constructible<ViewType>
	{
		return this->Base;
	}

	[[nodiscard]] constexpr ViewType base() && noexcept(std::is_nothrow_move_constructible_v<ViewType>) {
		return std::move(this->Base);
	}

	[[nodiscard]] constexpr auto begin() noexcept(std::is_nothrow_invocable_v<decltype(std::ranges::begin), ViewType&>)
	requires(!CommonExpositionOnly::SimpleView<ViewType>)
	{
		return Iterator<false>(std::ranges::begin(this->Base));
	}

	[[nodiscard]] constexpr auto begin() const noexcept(std::is_nothrow_invocable_v<decltype(std::ranges::begin), const ViewType&>)
	requires std::ranges::range<const ViewType>
	{
		return Iterator<true>(std::ranges::begin(this->Base));
	}

	[[nodiscard]] constexpr auto end() noexcept(std::is_nothrow_invocable_v<decltype(std::ranges::end), ViewType&>)
	requires(!CommonExpositionOnly::SimpleView<ViewType>)
	{
		return std::ranges::end(this->Base);
	}

	[[nodiscard]] constexpr auto end() const noexcept(std::is_nothrow_invocable_v<decltype(std::ranges::end), const ViewType&>)
	requires std::ranges::range<const ViewType>
	{
		return std::ranges::end(this->Base);
	}

	[[nodiscard]] constexpr auto size() noexcept(std::is_nothrow_invocable_v<decltype(std::ranges::size), ViewType&>)
	requires std::ranges::sized_range<ViewType>
	{
		return std::ranges::size(this->Base);
	}

	[[nodiscard]] constexpr auto size() const noexcept(std::is_nothrow_invocable_v<decltype(std::ranges::size), const ViewType&>)
	requires std::ranges::sized_range<const ViewType>
	{
		return std::ranges::size(this->Base);
	}

};

template<typename R>
ToInputView(R&&) -> ToInputView<std::views::all_t<R>>;
#endif//DRR_CORE_VIEW_HAS_STD_RANGES_TO_INPUT

/**
 * @brief ToInput range adaptor.
 */
inline constexpr auto ToInput =
#ifdef DRR_CORE_VIEW_HAS_STD_RANGES_TO_INPUT
std::views::to_input;
#else
RangeAdaptorClosure([]<std::ranges::viewable_range R>
	requires std::ranges::input_range<R>
	(R&& r) static constexpr noexcept(
		Trait::IsNothrowViewable<R>
		&& std::is_nothrow_constructible_v<ToInputView<std::views::all_t<R>>, R>
	) -> std::ranges::view auto {
		return ToInputView(std::forward<R>(r));
	});
#endif

}

#ifndef DRR_CORE_VIEW_HAS_STD_RANGES_TO_INPUT
template<typename V>
constexpr bool std::ranges::enable_borrowed_range<DisRegRep::Core::View::ToInputView<V>> = std::ranges::enable_borrowed_range<V>;
#endif