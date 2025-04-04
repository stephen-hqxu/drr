#pragma once

#include "../Protocol.hpp"
#include "../../Tiff.hpp"

#include <DisRegRep/Container/Regionfield.hpp>

template<>
struct DisRegRep::Image::Serialisation::Protocol<DisRegRep::Container::Regionfield> {

	using Serialisable = Container::Regionfield;

	static void initialise();
	static void read(const Tiff&, Serialisable&);
	static void write(const Tiff&, const Serialisable&, Tiff::ColourPaletteRandomEngineSeed);

};