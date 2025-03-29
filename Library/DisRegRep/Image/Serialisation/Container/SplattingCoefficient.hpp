#pragma once

#include "../Protocol.hpp"
#include "../../Tiff.hpp"

#include <DisRegRep/Container/SplattingCoefficient.hpp>

#include <cstdint>

template<>
struct DisRegRep::Image::Serialisation::Protocol<DisRegRep::Container::SplattingCoefficient::DenseMask> {

	using Serialisable = Container::SplattingCoefficient::DenseMask;
	using PixelType = std::uint16_t;

	static void write(const Tiff&, const Serialisable&);

};