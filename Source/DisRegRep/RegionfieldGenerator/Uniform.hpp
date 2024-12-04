#pragma once

#include "Base.hpp"

#include <DisRegRep/Container/Regionfield.hpp>

#include <string_view>

namespace DisRegRep::RegionfieldGenerator {

/**
 * @brief Generate a regionfield where region identifiers are distributed uniformly random.
*/
class Uniform final : public Base {
public:

	constexpr Uniform() noexcept = default;

	constexpr ~Uniform() override = default;

	[[nodiscard]] constexpr std::string_view name() const noexcept override {
		return "Uniform";
	}

	void operator()(Container::Regionfield&) override;

};

}