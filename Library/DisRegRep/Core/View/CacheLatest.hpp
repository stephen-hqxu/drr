#pragma once

#include "CommonExpositionOnly.hpp"
#include "RangeAdaptorClosure.hpp"
#include "Trait.hpp"

#include <optional>

#include <iterator>
#include <ranges>

#include <memory>
#include <utility>

#include <concepts>
#include <type_traits>

#include <version>

#if __cpp_lib_ranges_cache_latest >= 2024'11L
#define DRR_CORE_VIEW_HAS_STD_RANGES_CACHE_LATEST
#endif

namespace DisRegRep::Core::View {

#ifndef DRR_CORE_VIEW_HAS_STD_RANGES_CACHE_LATEST
/**
 * @brief Reference implementation of C++26 std::ranges::cache_latest_view.
 *
 * @ref https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3138r5.html
 *
 * @tparam V Range whose latest dereferenced value is cached.
 */
template<std::ranges::input_range V>
requires std::ranges::view<V>
class CacheLatestView : public std::ranges::view_interface<CacheLatestView<V>> {
public:

	using ViewType = V;

	using Value = std::ranges::range_value_t<ViewType>;
	using Difference = std::ranges::range_difference_t<ViewType>;
	using Reference = std::ranges::range_reference_t<ViewType>;
	using RValueReference = std::ranges::range_rvalue_reference_t<ViewType>;

	using ViewIterator = std::ranges::iterator_t<ViewType>;
	using ViewSentinel = std::ranges::sentinel_t<ViewType>;

	static constexpr bool IsReferenceReference = std::is_reference_v<Reference>;

private:

	using CacheValueType = std::conditional_t<
		IsReferenceReference,
		std::add_pointer_t<Reference>,
		Reference
	>;

	ViewType Base;
	std::optional<CacheValueType> Cache;

	class Iterator {
	public:

		friend CacheLatestView;

		using value_type = Value;
		using difference_type = Difference;
		using iterator_concept = std::input_iterator_tag;

	private:

		CacheLatestView* Parent;
		ViewIterator Current;

		explicit constexpr Iterator(CacheLatestView& parent)
			noexcept(std::is_nothrow_invocable_v<decltype(std::ranges::begin), ViewType&>) :
			Parent(std::addressof(parent)), Current(std::ranges::begin(parent.Base)) { }

	public:

		Iterator(const Iterator&) = delete;

		Iterator(Iterator&&) noexcept(std::is_nothrow_move_constructible_v<ViewIterator>) = default;

		Iterator& operator=(const Iterator&) = delete;

		Iterator& operator=(Iterator&&) noexcept(std::is_nothrow_move_assignable_v<ViewIterator>) = default;

		~Iterator() = default;

		[[nodiscard]] constexpr const ViewIterator& base() const & noexcept {
			return this->Current;
		}

		[[nodiscard]] constexpr ViewIterator base() && noexcept(std::is_nothrow_move_constructible_v<ViewIterator>) {
			return std::move(this->Current);
		}

		[[nodiscard]] constexpr Reference& operator*() const noexcept(std::is_nothrow_constructible_v<CacheValueType, Reference>) {
			using std::addressof,
				std::remove_reference_t, std::add_lvalue_reference_t;
			if constexpr (auto& cache = this->Parent->Cache;
				IsReferenceReference) {
				if (!cache) {
					cache = addressof(CommonExpositionOnly::asLValue(*this->Current));
				}
				return **cache;
			} else {
				if (!cache) {
					cache.emplace(*this->Current);
				}
				return *cache;
			}
		}

		constexpr Iterator& operator++() noexcept(noexcept(++this->Current)) {
			this->Parent->Cache.reset();
			++this->Current;
			return *this;
		}

		constexpr void operator++(int) noexcept(noexcept(++*this)) {
			++*this;
		}

		//NOLINTBEGIN(readability-identifier-naming)
		[[nodiscard]] friend constexpr RValueReference iter_move(const Iterator& it)
			noexcept(noexcept(std::ranges::iter_move(it.Current))) {
			return std::ranges::iter_move(it.Current);
		}

		friend constexpr void iter_swap(const Iterator& x, const Iterator& y)
			noexcept(noexcept(std::ranges::iter_swap(x.Current, y.Current)))
		requires std::indirectly_swappable<ViewIterator>
		{
			std::ranges::iter_swap(x.Current, y.Current);
		}
		//NOLINTEND(readability-identifier-naming)

	};

	class Sentinel {
	private:

		friend CacheLatestView;

		ViewSentinel End;

		explicit constexpr Sentinel(CacheLatestView& parent)
			noexcept(std::is_nothrow_invocable_v<decltype(std::ranges::end), ViewType&>) :
			End(std::ranges::end(parent.Base)) { }

	public:

		constexpr Sentinel() noexcept(std::is_nothrow_default_constructible_v<ViewSentinel>) = default;

		[[nodiscard]] constexpr ViewSentinel base() const noexcept(std::is_nothrow_copy_constructible_v<ViewSentinel>) {
			return this->End;
		}

		[[nodiscard]] friend constexpr bool operator==(const Iterator& x, const Sentinel& y) noexcept(noexcept(x.Current == y.End)) {
			return x.Current == y.End;
		}

		[[nodiscard]] friend constexpr Difference operator-(const Iterator& x, const Sentinel& y) noexcept(noexcept(x.Current - y.End))
		requires std::sized_sentinel_for<ViewSentinel, ViewIterator>
		{
			return x.Current - y.End;
		}

		[[nodiscard]] friend constexpr Difference operator-(const Sentinel& x, const Iterator& y) noexcept(noexcept(x.End - y.Current))
		requires std::sized_sentinel_for<ViewSentinel, ViewIterator>
		{
			return x.End - y.Current;
		}

	};

public:

	CacheLatestView() noexcept(std::is_nothrow_default_constructible_v<ViewType>)
	requires std::default_initializable<ViewType>
	= default;

	explicit constexpr CacheLatestView(ViewType base) noexcept(std::is_nothrow_move_constructible_v<ViewType>) :
		Base(std::move(base)) { }

	[[nodiscard]] constexpr ViewType base() const& noexcept(std::is_nothrow_copy_constructible_v<ViewType>)
	requires std::copy_constructible<ViewType>
	{
		return this->Base;
	}

	[[nodiscard]] constexpr ViewType base() && noexcept(std::is_nothrow_move_constructible_v<ViewType>) {
		return std::move(this->Base);
	}

	[[nodiscard]] constexpr auto begin() noexcept(std::is_nothrow_constructible_v<Iterator, CacheLatestView&>) {
		return Iterator(*this);
	}

	[[nodiscard]] constexpr auto end() noexcept(std::is_nothrow_constructible_v<Sentinel, CacheLatestView&>) {
		return Sentinel(*this);
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
CacheLatestView(R&&) -> CacheLatestView<std::views::all_t<R>>;
#endif//DRR_CORE_VIEW_HAS_STD_RANGES_CACHE_LATEST

/**
 * @brief CacheLatestView range adaptor.
 */
inline constexpr auto CacheLatest =
#ifdef DRR_CORE_VIEW_HAS_STD_RANGES_CACHE_LATEST
std::views::cache_latest;
#else
RangeAdaptorClosure([]<std::ranges::viewable_range R>(R&& r) static constexpr noexcept(
						noexcept(CacheLatestView(std::forward<R>(r)))) -> std::ranges::view auto
	requires requires { CacheLatestView(std::forward<R>(r)); }
	{
		return CacheLatestView(std::forward<R>(r));
	});
#endif

}