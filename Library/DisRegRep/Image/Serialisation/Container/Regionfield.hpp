#pragma once

#include "../Protocol.hpp"
#include "../../Tiff.hpp"

#include <DisRegRep/Container/Regionfield.hpp>

template<>
struct DisRegRep::Image::Serialisation::Protocol::Implementation<DisRegRep::Container::Regionfield> {

	using Serialisable = Container::Regionfield;

	struct WriteInfo {

		CompressionScheme::Option Compression;
		Tiff::ColourPaletteRandomEngineSeed Seed; /**< Seed for random generation of regionfield colour palette for display purposes. */

	};

	static void initialise();
	static void read(const Tiff&, Serialisable&);
	static void write(const Tiff&, const Serialisable&, const WriteInfo&);

};