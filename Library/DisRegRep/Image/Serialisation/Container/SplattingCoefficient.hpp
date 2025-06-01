#pragma once

#include "../Protocol.hpp"
#include "../../Tiff.hpp"

#include <DisRegRep/Container/SplattingCoefficient.hpp>

#include <span>

#include <cstdint>

/**
 * @brief Support writing multiple splatting coefficient matrices into a single image. Each matrix can be assigned with an
 * implementation-defined identifier.
 */
template<>
struct DisRegRep::Image::Serialisation::Protocol::Implementation<DisRegRep::Container::SplattingCoefficient::DenseMask> {

	using Serialisable = Container::SplattingCoefficient::DenseMask;
	using PixelType = std::uint16_t;
	using IdentifierType = std::uint_fast8_t;

	struct WriteInfo {

		CompressionScheme::Option Compression;

	};

	static void initialise();
	static void write(const Tiff&, const Serialisable&, IdentifierType, const WriteInfo&);
	static void write(const Tiff&, std::span<const Serialisable* const>, std::span<const IdentifierType>, const WriteInfo&);

};