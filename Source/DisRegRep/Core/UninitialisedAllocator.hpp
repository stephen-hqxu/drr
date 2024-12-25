#pragma once

#include <memory>
#include <type_traits>

namespace DisRegRep {

/**
 * @brief An allocator adaptor that does not perform construction nor destruction on a compatible type. This allows the type to be
 * uninitialised and provides performance benefits when used in STL containers.
 *
 * @tparam T Type of value to be allocated.
 * @tparam Alloc An existing allocator.
 */
template<typename T, typename Alloc = std::allocator<T>>
requires std::is_same_v<T, typename std::allocator_traits<Alloc>::value_type>
class UninitialisedAllocator : public Alloc {
public:

	using BaseAllocator = Alloc;
	using BaseAllocatorTrait = std::allocator_traits<BaseAllocator>;
	template<typename U>
	using RebindBaseAllocator = typename std::allocator_traits<BaseAllocator>::template rebind_alloc<U>;

	template<typename U>
	struct rebind {

		using other = std::conditional_t<
			std::is_same_v<typename BaseAllocatorTrait::value_type, U>,
			UninitialisedAllocator,
			RebindBaseAllocator<U>
		>;

	};

	using BaseAllocator::BaseAllocator;

	template<typename U>
	requires std::is_trivially_default_constructible_v<U>
	constexpr void construct(U*) const noexcept { }

	template<typename U>
	requires std::is_trivially_destructible_v<U>
	constexpr void destroy(U*) const noexcept { }

};

}