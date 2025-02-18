#pragma once

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

namespace DisRegRep::Core::View {

/**
 * @brief Reference implementation of C++26 std::views::cache_latest.
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

	ViewType Base = V();
	std::optional<CacheValueType> Cache;

	class Iterator {
	public:

		friend class CacheLatestView;

		using value_type = Value;
		using difference_type = Difference;
		using iterator_concept = std::input_iterator_tag;

	private:

		CacheLatestView* Parent;
		ViewIterator Current;

		explicit constexpr Iterator(CacheLatestView& parent)
			noexcept(std::is_nothrow_invocable_v<decltype(std::ranges::begin), ViewType>) :
			Parent(std::addressof(parent)), Current(std::ranges::begin(parent.Base)) { }

	public:

		Iterator(const Iterator&) noexcept(std::is_nothrow_copy_constructible_v<ViewIterator>) = default;

		Iterator(Iterator&&) noexcept(std::is_nothrow_move_constructible_v<ViewIterator>) = default;

		Iterator& operator=(const Iterator&) noexcept(std::is_nothrow_copy_assignable_v<ViewIterator>) = default;

		Iterator& operator=(Iterator&&) noexcept(std::is_nothrow_move_assignable_v<ViewIterator>) = default;

		~Iterator() = default;

		[[nodiscard]] constexpr const ViewIterator& base() const & noexcept {
			return this->Current;
		}

		[[nodiscard]] constexpr ViewIterator base() && noexcept(std::is_nothrow_move_constructible_v<ViewIterator>) {
			return std::move(this->Current);
		}

		[[nodiscard]] constexpr Reference& operator*() const {
			using std::addressof,
				std::remove_reference_t, std::add_lvalue_reference_t;
			if constexpr (auto& cache = this->Parent->Cache;
				IsReferenceReference) {
				if (!cache) {
					cache = addressof(static_cast<add_lvalue_reference_t<remove_reference_t<Reference>>>(*this->Current));
				}
				return **cache;
			} else {
				if (!cache) {
					cache.emplace(*this->Current);
				}
				return *cache;
			}
		}

		constexpr Iterator& operator++() {
			this->Parent->Cache.reset();
			++this->Current;
			return *this;
		}

		constexpr void operator++(int) {
			++*this;
		}

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

	};

	class Sentinel {
	private:

		friend class CacheLatestView;

		ViewSentinel End = ViewSentinel();

		explicit constexpr Sentinel(CacheLatestView& parent)
			noexcept(std::is_nothrow_invocable_v<decltype(std::ranges::end), ViewType>) :
			End(std::ranges::end(parent.Base)) { }

	public:

		constexpr Sentinel() noexcept(std::is_nothrow_default_constructible_v<ViewSentinel>) = default;

		Sentinel(const Sentinel&) noexcept(std::is_nothrow_copy_constructible_v<ViewSentinel>) = default;

		Sentinel(Sentinel&&) noexcept(std::is_nothrow_move_constructible_v<ViewSentinel>) = default;

		Sentinel& operator=(const Sentinel&) noexcept(std::is_nothrow_copy_assignable_v<ViewSentinel>) = default;

		Sentinel& operator=(Sentinel&&) noexcept(std::is_nothrow_move_assignable_v<ViewSentinel>) = default;

		~Sentinel() = default;

		[[nodiscard]] constexpr ViewSentinel base() const noexcept(std::is_nothrow_copy_constructible_v<ViewSentinel>) {
			return this->End;
		}

		[[nodiscard]] friend constexpr bool operator==(const Iterator& x, const Sentinel& y) {
			return x.Current == y.End;
		}

		[[nodiscard]] friend constexpr Difference operator-(const Iterator& x, const Sentinel& y)
		requires std::sized_sentinel_for<ViewSentinel, ViewIterator>
		{
			return x.Current - y.End;
		}

		[[nodiscard]] friend constexpr Difference operator-(const Sentinel& x, const Iterator& y)
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

	CacheLatestView(const CacheLatestView&)
		noexcept(std::is_nothrow_copy_constructible_v<ViewType> && std::is_nothrow_copy_constructible_v<CacheValueType>) = default;

	CacheLatestView(CacheLatestView&&)
		noexcept(std::is_nothrow_move_constructible_v<ViewType> && std::is_nothrow_move_constructible_v<CacheValueType>) = default;

	CacheLatestView& operator=(const CacheLatestView&)
		noexcept(std::is_nothrow_copy_assignable_v<ViewType> && std::is_nothrow_copy_assignable_v<CacheValueType>) = default;

	CacheLatestView& operator=(CacheLatestView&&)
		noexcept(std::is_nothrow_move_assignable_v<ViewType> && std::is_nothrow_move_assignable_v<CacheValueType>) = default;

	~CacheLatestView() = default;

	[[nodiscard]] constexpr ViewType base() const & noexcept(std::is_nothrow_copy_constructible_v<ViewType>)
	requires std::copy_constructible<ViewType>
	{
		return this->Base;
	}

	[[nodiscard]] constexpr ViewType base() && noexcept(std::is_nothrow_move_constructible_v<ViewType>) {
		return std::move(this->Base);
	}

	[[nodiscard]] constexpr auto begin() {
		return Iterator(*this);
	}

	[[nodiscard]] constexpr auto end() {
		return Sentinel(*this);
	}

	[[nodiscard]] constexpr auto size()
	requires std::ranges::sized_range<ViewType>
	{
		return std::ranges::size(this->Base);
	}

	[[nodiscard]] constexpr auto size() const
	requires std::ranges::sized_range<const ViewType>
	{
		return std::ranges::size(this->Base);
	}

};

template<typename R>
CacheLatestView(R&&) -> CacheLatestView<std::views::all_t<R>>;

/**
 * @brief CacheLatestView range adaptor.
 */
inline constexpr auto CacheLatest =
#if __cpp_lib_ranges_cache_latest == 2024'11L
std::views::cache_latest
#else
RangeAdaptorClosure([]<std::ranges::viewable_range R>
	requires std::ranges::input_range<R>
	(R&& r) static constexpr noexcept(Trait::IsNothrowViewable<R>) -> std::ranges::view auto {
		return CacheLatestView(std::forward<R>(r));
	});
#endif

}