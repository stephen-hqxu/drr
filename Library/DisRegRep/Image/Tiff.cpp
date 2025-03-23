#include <DisRegRep/Image/Tiff.hpp>
#include <DisRegRep/Image/Error.hpp>

#include <DisRegRep/Core/Exception.hpp>

#include <DisRegRep/Info.hpp>

#include <tiff.h>
#include <tiffio.h>

#include <array>
#include <string_view>
#include <tuple>

#include <chrono>
#include <format>
#include <memory>
#include <utility>

#include <cassert>
#include <cstdint>

using DisRegRep::Image::Tiff;

using std::array, std::string_view,
	std::tuple, std::apply;
using std::chrono::system_clock, std::chrono::time_point_cast, std::chrono::sys_seconds,
	std::format, std::format_to;

void Tiff::Close::operator()(TIFF* const tif) noexcept {
	TIFFClose(tif);
}

template<typename... Arg>
void Tiff::setFieldGeneric(const Tag tag, Arg&&... arg) const {
	assert(*this);
	//NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
	DRR_ASSERT_IMAGE_TIFF_ERROR(TIFFSetField(**this, tag, std::forward<Arg>(arg)...));
}

template<typename... Arg>
tuple<Arg...> Tiff::getFieldGeneric(const Tag tag) const {
	assert(*this);

	tuple<Arg...> value;
	//NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
	apply([tif = **this, tag](auto&... v) { DRR_ASSERT_IMAGE_TIFF_ERROR(TIFFGetField(tif, tag, std::addressof(v)...)); }, value);
	return value;
}

//NOLINTNEXTLINE(bugprone-suspicious-stringview-data-usage)
Tiff::Tiff(const string_view filename, const string_view mode) : Handle(TIFFOpen(filename.data(), mode.data())) {
	DRR_ASSERT(*this);
}

void Tiff::setField(const Tag tag, const std::string_view value) const {
	this->setFieldGeneric(tag, value.data());
}

void Tiff::setField(const Tag tag, const std::uint16_t value) const {
	this->setFieldGeneric(tag, auto(value));
}

void Tiff::setColourPalette(const ConstColourPalette& palette) const {
	DRR_ASSERT(this->getFieldUInt16(TIFFTAG_PHOTOMETRIC) == PHOTOMETRIC_PALETTE);
	apply(
		[this, size = 1UZ << this->getFieldUInt16(TIFFTAG_BITSPERSAMPLE)](const auto&... channel) {
			DRR_ASSERT(((channel.size() == size) && ...));
			this->setFieldGeneric(TIFFTAG_COLORMAP, channel.data()...);
		},
		palette
	);
}

std::uint16_t Tiff::getFieldUInt16(const Tag tag) const {
	return std::get<0U>(this->getFieldGeneric<std::uint16_t>(tag));
}

void Tiff::setDefaultMetadata() const {
	array<string_view::value_type, 20U> datetime;
	format_to(datetime.begin(), "{:%Y:%m:%d %H:%M:%S}", time_point_cast<sys_seconds::duration>(system_clock::now()));
	datetime.back() = {};

	this->setField(TIFFTAG_SOFTWARE, Info::VersionLine);
	this->setField(TIFFTAG_COPYRIGHT,
		format("For comprehensive copyright information, please refer to the license available at {}.", Info::HomePage));
	this->setField(TIFFTAG_DATETIME, string_view(datetime.data(), datetime.size()));
}