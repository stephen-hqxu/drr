#pragma once

#include <ranges>
#include <concepts>

namespace DisRegRep {

/**
 * @brief Some traits for container types.
*/
namespace ContainerTrait {

//Check if a type is an instance of a container.
template<typename T, template<typename> class C>
concept ContainerInstance = requires {
	{ C<typename T::value_type> { } } -> std::same_as<T>;
};

//Check if a range type is the target.
template<typename R, typename T>
concept ValueRange = std::same_as<std::ranges::range_value_t<R>, T>;

}

}