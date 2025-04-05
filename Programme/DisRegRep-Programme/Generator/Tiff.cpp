#include <DisRegRep-Programme/Generator/Tiff.hpp>

#include <DisRegRep/Image/Tiff.hpp>

#include <tiff.h>

#include <variant>

namespace TifGen = DisRegRep::Programme::Generator::Tiff;
using DisRegRep::Image::Tiff;

using std::visit;

namespace {

void setCompression(const Tiff& tif, TifGen::Compression::None) {
	tif.setField(TIFFTAG_COMPRESSION, COMPRESSION_NONE);
}
void setCompression(const Tiff& tif, TifGen::Compression::LempelZivWelch) {
	tif.setField(TIFFTAG_COMPRESSION, COMPRESSION_LZW);
}
void setCompression(const Tiff& tif, const TifGen::Compression::ZStandard option) {
	const auto [compression_level] = option;
	tif.setField(TIFFTAG_COMPRESSION, COMPRESSION_ZSTD);
	tif.setField(TIFFTAG_ZSTD_LEVEL, compression_level);
}

}

void TifGen::setCompression(const Image::Tiff& tif, const Compression::Option& option) {
	visit([&tif](const auto& compression) { ::setCompression(tif, compression); }, option);
}