#pragma once

#include <DisRegRep/Image/Tiff.hpp>

#include <variant>

/**
 * @brief A high-level interface to interact with a TIFF image handle.
 */
namespace DisRegRep::Programme::Generator::Tiff {

/**
 * @brief Configure how data stored in a TIFF image should be compressed.
 */
namespace Compression {

/**
 * @brief Perform no compression at all.
 */
struct None { };
/**
 * @brief Use the Lempel-Ziv & Welch (a.k.a. LZW) compression algorithm. This algorithm is supported by most TIFF readers thus providing
 * a reasonable portability across different programmes. Nonetheless, it may underperform and lead to increased size when input is too
 * random.
 */
struct LempelZivWelch { };
/**
 * @brief Use the Z-Standard (a.k.a. zstd) compression algorithm. This algorithm typically yields the best compression ratio, yet
 * portability with other TIFF readers is relatively poor due to its non-standardisation.
 *
 * @note *libtiff* must be built with ZStandard algorithm enabled.
 */
struct ZStandard {

	int CompressionLevel; /**< zstd compression level. Please refer to their documentation regarding the valid range. */

};

using Option = std::variant<
	None,
	LempelZivWelch,
	ZStandard
>; /**< All supported compression algorithms and their configurations. */

}

/**
 * @brief Set the compression level of a handle.
 *
 * @param tif Handle whose compression level is to be set.
 * @param option Compression option.
 */
void setCompression(const Image::Tiff&, const Compression::Option&);

}