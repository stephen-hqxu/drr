#pragma once

#include <range/v3/view/cache1.hpp>
#include <range/v3/view/transform.hpp>

#include <functional>
#include <iterator>
#include <ranges>

#include <utility>

#include <concepts>
#include <type_traits>

/**
 * @brief Additional range views and adaptors.
 */
namespace DisRegRep::Core::Range {

/**
 * `F` can be used as a range function.
 */
template<typename F>
concept RangeApplicator = std::move_constructible<F> && std::is_object_v<F>;

/**
 * @brief Common range adaptor closure that takes a single range as argument.
 *
 * @library STL
 * 
 * @tparam F Range functor that consumes a range.
 */
template<RangeApplicator F>
class RangeAdaptorClosure : public std::ranges::range_adaptor_closure<RangeAdaptorClosure<F>> {
public:

	using FunctionType = F;
	using Const = std::add_const_t<FunctionType>;
	using ConstReference = std::add_lvalue_reference_t<Const>;

private:

	FunctionType Function;

public:

	/**
	 * @brief Create a range adaptor closure.
	 *
	 * @param f Range adaptor closure functor.
	 */
	explicit constexpr RangeAdaptorClosure(F f) noexcept(std::is_nothrow_move_constructible_v<FunctionType>) :
		Function(std::move(f)) { }

	constexpr ~RangeAdaptorClosure() = default;

	/**
	 * @brief Invoke RAC with a range.
	 *
	 * @tparam R A range type.
	 * @param r Range to be passed to the function.
	 *
	 * @return Result of `f(r)`.
	 */
	template<std::ranges::range R>
	requires std::is_invocable_v<ConstReference, R>
	[[nodiscard]] constexpr auto operator()(R&& r) const noexcept(std::is_nothrow_invocable_v<ConstReference, R>) {
		return std::invoke(this->Function, std::forward<R>(r));
	}

	/**
	 * @brief Try to invoke RAC with many arguments. If not possible, that is mostly because the function is a range adaptor, and a RAC
	 * will be created.
	 *
	 * @tparam Arg Call argument type.
	 * @param arg Argument to be used to invoke the function.
	 *
	 * @return Result of `f(arg...)`, or a RAC.
	 */
	template<typename... Arg>
	[[nodiscard]] constexpr auto operator()(Arg&&... arg) const
		//The noexcept specifier for bind_back is too complex, and I am too lazy to write it; let the compiler deduce itself.
		noexcept(std::is_invocable_v<ConstReference, Arg...> ? std::is_nothrow_invocable_v<ConstReference, Arg...> : false) {
		if constexpr (std::is_invocable_v<ConstReference, Arg...>) {
			return std::invoke(this->Function, std::forward<Arg>(arg)...);
		} else {
			auto rac = std::bind_back(this->Function, std::forward<Arg>(arg)...);
			return RangeAdaptorClosure<decltype(rac)>(std::move(rac));
		}
	}

};

/**
 * @brief Apply a transformation to each element in the range. Differ from `std::views::transform`, the transformation function is
 * allowed to be impure. The resulting range is single-pass due to caching.
 *
 * @library range-v3
 *
 * @tparam R Input range type.
 * @tparam F Transformation function type.
 *
 * @param r Input range whose elements are to be transformed.
 * @param f Transformation function to be applied.
 *
 * @return Transformed range.
 */
inline constexpr auto ImpureTransform = RangeAdaptorClosure([]<std::ranges::viewable_range R, RangeApplicator F>
	requires std::ranges::input_range<R> && std::indirectly_unary_invocable<F, std::ranges::iterator_t<R>>
	(R&& r, F f) static constexpr noexcept(
		std::is_nothrow_constructible_v<std::decay_t<R>, R>&& std::is_nothrow_move_constructible_v<F>) -> auto {
		//This cache views force the range to be an input range, and each element in the preceding range will be dereferenced exactly
		//	once, thus eliminating the possibility of calling the impure function more than once per iteration.
		return std::forward<R>(r) | ranges::views::transform(std::move(f)) | ranges::views::cache1;
	});

}