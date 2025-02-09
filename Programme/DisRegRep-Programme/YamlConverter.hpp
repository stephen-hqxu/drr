#pragma once

#include <yaml-cpp/node/convert.h>
#include <yaml-cpp/node/node.h>

#include <glm/fwd.hpp>

#include <utility>

#include <cstddef>

namespace YAML {

template<glm::length_t L, typename T, glm::qualifier Q>
struct convert<glm::vec<L, T, Q>> {

	using ConvertType = glm::vec<L, T, Q>;

	static bool decode(const Node& node, ConvertType& vec) {
		if (!(node.IsSequence() && node.size() == L)) [[unlikely]] {
			return false;
		}
		using std::index_sequence, std::make_index_sequence;
		[&]<std::size_t... I>(index_sequence<I...>) {
			((vec[I] = node[I].template as<T>()), ...);
		}(make_index_sequence<L> {});
		return true;
	}

};

}