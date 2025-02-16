#pragma once

#include <DisRegRep/Container/SparseMatrixElement.hpp>

#include <catch2/catch_tostring.hpp>

#include <glm/gtc/type_ptr.hpp>
#include <glm/fwd.hpp>

#include <span>

#include <format>
#include <string>

#include <concepts>

namespace Catch {

template<glm::length_t L, typename T, glm::qualifier Q>
struct StringMaker<glm::vec<L, T, Q>> {

	using MakeType = glm::vec<L, T, Q>;

	static std::string convert(const MakeType& v) {
		return std::format("({:n})", std::span<const T, L>(glm::value_ptr(v), L));
	}

};

template<std::floating_point V>
struct StringMaker<DisRegRep::Container::SparseMatrixElement::Basic<V>> {

	using MakeType = DisRegRep::Container::SparseMatrixElement::Basic<V>;

	static std::string convert(const MakeType& mat_elem) {
		const auto [region_id, value] = mat_elem;
		return std::format("{{Identifier: {}, Value: {:.2f}}}", region_id, value);
	}

};

}