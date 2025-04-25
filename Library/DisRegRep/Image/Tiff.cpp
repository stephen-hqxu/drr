#include <DisRegRep/Image/Tiff.hpp>

#include <DisRegRep/Core/Bit.hpp>
#include <DisRegRep/Core/Exception.hpp>

#include <DisRegRep/Info.hpp>

#include <glm/fwd.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <tiff.h>
#include <tiffio.h>

#include <array>
#include <span>

#include <string_view>
#include <tuple>

#include <algorithm>
#include <ranges>

#include <bit>
#include <chrono>
#include <format>
#include <memory>
#include <utility>

#include <limits>

#include <cstddef>
#include <cstdint>

using DisRegRep::Image::Tiff;

using glm::f32vec2, glm::u32vec3;

using std::array, std::to_array,
	std::string_view, std::span,
	std::apply;
using std::ranges::for_each, std::ranges::copy, std::ranges::transform,
	std::views::chunk;
using std::chrono::system_clock, std::chrono::time_point_cast, std::chrono::sys_seconds,
	std::format, std::format_to,
	std::make_unique_for_overwrite,
	std::integer_sequence, std::make_integer_sequence;
using std::numeric_limits;

namespace {

namespace TiffTag {

constexpr auto ImageExtent = to_array<Tiff::Tag>({ TIFFTAG_IMAGEWIDTH, TIFFTAG_IMAGELENGTH, TIFFTAG_IMAGEDEPTH });
constexpr auto TileExtent = to_array<Tiff::Tag>({ TIFFTAG_TILEWIDTH, TIFFTAG_TILELENGTH, TIFFTAG_TILEDEPTH });
constexpr auto Resolution = to_array<Tiff::Tag>({ TIFFTAG_XRESOLUTION, TIFFTAG_YRESOLUTION });

}

}

void Tiff::Close::operator()(TIFF* const tif) noexcept {
	TIFFClose(tif);
}

//NOLINTNEXTLINE(bugprone-suspicious-stringview-data-usage)
Tiff::Tiff(const string_view filename, const string_view mode) : Handle(TIFFOpen(filename.data(), mode.data())) {
	DRR_ASSERT(*this);
}

void Tiff::setColourPalette(const ConstColourPalette& palette) const {
	DRR_ASSERT(this->getField<std::uint16_t>(TIFFTAG_PHOTOMETRIC) == PHOTOMETRIC_PALETTE);
	apply(
		[this, size = this->getColourPaletteSize()](const auto&... channel) {
			DRR_ASSERT(((channel.size() == size) && ...));
			this->setField(TIFFTAG_COLORMAP, channel.data()...);
		},
		palette
	);
}

void Tiff::setColourPalette(const ColourPaletteRandomEngineSeed seed) const {
	using Limit = numeric_limits<ColourPaletteElement>;
	static_assert(ColourPaletteRandomEngine::min() == 0U && std::countr_one(ColourPaletteRandomEngine::max()) == Limit::digits * 3U,
		"Choose a random number generator whose range can cover exactly the size of three colour palette channels to ensure maximum instruction level parallelism.");

	const auto palette_size = this->getColourPaletteSize(),
		rgb_palette_size = palette_size * 3U;
	const auto palette_ptr = make_unique_for_overwrite<ColourPaletteElement[]>(rgb_palette_size);
	const auto palette = span(palette_ptr.get(), rgb_palette_size);
	for_each(palette | chunk(3U), [rng = ColourPaletteRandomEngine(seed)](const auto rgb) mutable {
		using Core::Bit::BitPerSampleResult;
		using EngineResult = ColourPaletteRandomEngine::result_type;
		static constexpr auto RGBBpsResult = BitPerSampleResult(BitPerSampleResult::DataTypeTag<EngineResult>, Limit::digits);
		//Unpack starts from MSB, shift up to fill in the gap.
		copy(Core::Bit::unpack(rng() << RGBBpsResult.Bit, rgb.size(), RGBBpsResult), rgb.begin());
	});

	ConstColourPalette palette_split;
	transform(palette | chunk(palette_size), palette_split.begin(), [](const auto channel) static constexpr { return span(channel); });
	this->setColourPalette(palette_split);
}

void Tiff::setImageExtent(const u32vec3 extent) const {
	[this, &extent]<u32vec3::length_type... I>(integer_sequence<u32vec3::length_type, I...>) {
		(this->setField(TiffTag::ImageExtent[I], extent[I]), ...);
	}(make_integer_sequence<u32vec3::length_type, u32vec3::length()> {});
}

void Tiff::setResolution(const f32vec2 resolution) const {
	[this, &resolution]<f32vec2::length_type... I>(integer_sequence<f32vec2::length_type, I...>) {
		(this->setField(TiffTag::Resolution[I], resolution[I]), ...);
	}(make_integer_sequence<f32vec2::length_type, f32vec2::length()> {});
}

void Tiff::setOptimalTileExtent() const {
	assert(*this);

	auto tw = std::numeric_limits<std::uint32_t>::max(), th = tw;
	TIFFDefaultTileSize(**this, &tw, &th);
	this->setField(TiffTag::TileExtent[0], tw);
	this->setField(TiffTag::TileExtent[1], th);
}

std::size_t Tiff::getColourPaletteSize() const {
	return 1UZ << this->getField<std::uint16_t>(TIFFTAG_BITSPERSAMPLE).value();
}

u32vec3 Tiff::getImageExtent() const {
	return apply(
		[this](const auto... tag) { return u32vec3(this->getField<u32vec3::value_type>(tag).value_or(1U)...); }, TiffTag::ImageExtent);
}

u32vec3 Tiff::getTileExtent() const {
	return apply(
		[this](const auto... tag) { return u32vec3(this->getField<u32vec3::value_type>(tag).value_or(1U)...); }, TiffTag::TileExtent);
}

void Tiff::setDefaultMetadata(const string_view description) const {
	array<string_view::value_type, 20U> datetime;
	format_to(datetime.begin(), "{:%Y:%m:%d %H:%M:%S}", time_point_cast<sys_seconds::duration>(system_clock::now()));
	datetime.back() = {};

	this->setField(TIFFTAG_SOFTWARE, Info::VersionLine.data());
	this->setField(TIFFTAG_COPYRIGHT,
		format("For comprehensive copyright information, please refer to the license available at {}.", Info::HomePage).c_str());
	this->setField(TIFFTAG_DATETIME, datetime.data());
	this->setField(TIFFTAG_IMAGEDESCRIPTION, description.data());
}

bool Tiff::isTiled() const noexcept {
	assert(*this);
	return TIFFIsTiled(**this);
}

std::size_t Tiff::tileSize() const {
	assert(*this);

	const tmsize_t size = TIFFTileSize(**this);
	DRR_ASSERT(size != 0);
	return size;
}

void Tiff::readTile(const BinaryBuffer buffer, const u32vec3 coordinate, const std::uint16_t sample) const {
	assert(*this);
	DRR_ASSERT(TIFFReadTile(**this, buffer.data(), coordinate.x, coordinate.y, coordinate.z, sample) != -1);
}

void Tiff::writeTile(const BinaryBuffer buffer, const u32vec3 coordinate, const std::uint16_t sample) const {
	assert(*this);
	DRR_ASSERT(TIFFWriteTile(**this, buffer.data(), coordinate.x, coordinate.y, coordinate.z, sample) != -1);
}

void Tiff::writeDirectory() const {
	assert(*this);
	DRR_ASSERT(TIFFWriteDirectory(**this) == 1);
}