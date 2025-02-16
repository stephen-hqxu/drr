#pragma once

#include <glm/fwd.hpp>

#include <mdspan>
#include <tuple>

#include <utility>

#include <cstddef>

/**
 * @brief Utilities and adaptors for @link std::mdspan. Also provides some interoperability with @link glm library.
 */
namespace DisRegRep::Core::MdSpan {

/**
 * @brief Convert a @link glm::vec to a @link std::extents.
 *
 * @tparam L Vector length.
 * @tparam T Index type.
 * @tparam Q Type qualifier.
 *
 * @param vec Input vector.
 *
 * @return Output extent.
 */
template<glm::length_t L, typename T, glm::qualifier Q>
[[nodiscard]] constexpr auto toExtent(const glm::vec<L, T, Q>& vec) noexcept {
	using std::integer_sequence, std::make_integer_sequence, std::dextents;
	return [&vec]<T... I>(integer_sequence<T, I...>) constexpr noexcept {
		return dextents<T, L>(vec[I]...);
	}(make_integer_sequence<T, L> {});
}

/**
 * @brief Convert a @link std::extents to a @link glm::vec.
 *
 * @tparam T Index type.
 * @tparam Extent Extent of each rank.
 *
 * @param ext Input extent.
 *
 * @return Output vector.
 */
template<typename T, std::size_t... Extent>
[[nodiscard]] constexpr auto toVector(const std::extents<T, Extent...>& ext) noexcept {
	using std::integer_sequence, std::make_integer_sequence, glm::vec;
	return [&ext]<T... I>(integer_sequence<T, I...>) constexpr noexcept {
		return vec<sizeof...(Extent), T>(ext.extent(I)...);
	}(make_integer_sequence<T, sizeof...(Extent)> {});
}

/**
 * @brief Flip the order of extents.
 *
 * @tparam T Index type.
 * @tparam Extent Extent of each rank.
 *
 * @param ext Input extent.
 *
 * @return Extent whose rank sizes are flipped.
 */
template<typename T, std::size_t... Extent>
[[nodiscard]] constexpr auto flip(const std::extents<T, Extent...>& ext) noexcept {
	using std::tuple, std::tuple_element_t, std::integral_constant,
		std::integer_sequence, std::make_integer_sequence, std::extents;
	//LANGUAGE-FEATURE: Pack indexing.
	return [&ext]<T... I>(integer_sequence<T, I...>) constexpr noexcept {
		using ExtentPack = tuple<integral_constant<std::size_t, Extent>...>;
		return extents<T, tuple_element_t<sizeof...(Extent) - 1U - I, ExtentPack>::value...>(
			ext.extent(sizeof...(Extent) - 1U - I)...);
	}(make_integer_sequence<T, sizeof...(Extent)> {});
}

}