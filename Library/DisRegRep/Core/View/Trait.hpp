#pragma once

#include <ranges>
#include <type_traits>

/**
 * @brief Extra type traits for views.
 */
namespace DisRegRep::Core::View::Trait {

/**
 * @brief Specify if a view can be created from a range with noexcept.
 *
 * @tparam R Range that might be potentially nothrow viewable. This may carry qualifiers from a forwarding reference.
 */
template<std::ranges::viewable_range R>
inline constexpr bool IsNothrowViewable = std::is_nothrow_constructible_v<std::views::all_t<R>, R>;

}