#include <DisRegRep/Image/Serialisation/Protocol.hpp>
#include <DisRegRep/Image/Tiff.hpp>

#include <tiff.h>

#include <variant>

#include <utility>

namespace Ptc = DisRegRep::Image::Serialisation::Protocol;
using DisRegRep::Image::Tiff;

using std::visit;

namespace {

void setCompressionScheme(const Tiff& tif, Ptc::CompressionScheme::None) {
	tif.setField(TIFFTAG_COMPRESSION, COMPRESSION_NONE);
}
void setCompressionScheme(const Tiff& tif, Ptc::CompressionScheme::LempelZivWelch) {
	tif.setField(TIFFTAG_COMPRESSION, COMPRESSION_LZW);
}
void setCompressionScheme(const Tiff& tif, const Ptc::CompressionScheme::ZStandard scheme) {
	const auto [level] = scheme;
	tif.setField(TIFFTAG_COMPRESSION, COMPRESSION_ZSTD);
	tif.setField(TIFFTAG_ZSTD_LEVEL, level);
}

}

void Ptc::setCompressionScheme(const Tiff& image, CompressionScheme::Option scheme) {
	visit([&image](auto scheme) { ::setCompressionScheme(image, std::move(scheme)); }, std::move(scheme));
}