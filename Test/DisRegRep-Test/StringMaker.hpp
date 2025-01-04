#pragma once

#include <catch2/catch_tostring.hpp>

#include <glm/fwd.hpp>

#include <string>

#include <format>

#include <utility>

#include <cstddef>

namespace std {

template<glm::length_t L, typename T, glm::qualifier Q>
struct formatter<glm::vec<L, T, Q>> : formatter<string> {

	auto format(const glm::vec<L, T, Q>& v, format_context& ctx) const {
		return this->formatter<string>::format(
			[&v]<std::size_t... I>(index_sequence<I...>) {
				string s;
				s.push_back('(');
				s.append(((to_string(v[I]) + ", ") + ...));
				//Remove the trailing comma.
				s.replace(s.size() - 2U, 2U, 1U, ')');
				return s;
			}(make_index_sequence<L> {}), ctx);
	}

};

}

namespace Catch {

template<glm::length_t L, typename T, glm::qualifier Q>
struct StringMaker<glm::vec<L, T, Q>> {

	static std::string convert(const glm::vec<L, T, Q>& v) {
		return std::format("{}", v);
	}

};

}