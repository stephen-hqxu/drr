#pragma once

#include <yaml-cpp/yaml.h>

#include <glm/fwd.hpp>

#include <utility>

namespace YAML {

template<glm::length_t L, typename T, glm::qualifier Q>
struct convert<glm::vec<L, T, Q>> {

	using ConvertType = glm::vec<L, T, Q>;

	static bool decode(const Node& node, ConvertType& vec) {
		if (!(node.IsSequence() && node.size() == L)) [[unlikely]] {
			return false;
		}
		using std::integer_sequence, std::make_integer_sequence;
		using LengthType = typename ConvertType::length_type;
		[&]<LengthType... I>(integer_sequence<LengthType, I...>) {
			((vec[I] = node[I].template as<T>()), ...);
		}(make_integer_sequence<LengthType, L> {});
		return true;
	}

};

}