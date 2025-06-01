#pragma once

#include "../Tiff.hpp"

#include <variant>

/**
 * @brief Define a serialisation protocol for a data structure that allows it to be serialised to an image. The application has freedom
 * controlling the definition of such protocol.
 */
namespace DisRegRep::Image::Serialisation::Protocol {

/**
 * @brief Configure how data stored in an image should be compressed.
 */
namespace CompressionScheme {

/**
 * @brief Perform no compression at all.
 */
struct None { };
/**
 * @brief Use the Lempel-Ziv & Welch (a.k.a. LZW) compression algorithm. This algorithm is supported by most image readers thus
 * providing a reasonable portability across different programmes. Nonetheless, it may underperform and lead to increased size when
 * input is too random.
 */
struct LempelZivWelch { };
/**
 * @brief Use the Z-Standard (a.k.a. zstd) compression algorithm. This algorithm typically yields the best compression ratio, yet
 * portability with other image readers is relatively poor due to its non-standardisation.
 *
 * @note For TIFF, *libtiff* must be built with *libzstd*.
 */
struct ZStandard {

	/**
	 * @brief *zstd* compression level. Please refer to their documentation regarding the valid range. Default to the level used by the
	 * official zstd command line.
	 */
	int Level = 3;

};

using Option = std::variant<
	None,
	LempelZivWelch,
	ZStandard
>; /**< All supported compression schemes and their configurations. */

}

/**
 * @brief Implement a serialisation protocol.
 */
template<typename>
struct Implementation;

/**
 * @brief Set the compression scheme of an image handle.
 *
 * @param image A handle whose compression scheme is set.
 * @param scheme Compression scheme.
 */
void setCompressionScheme(const Tiff&, CompressionScheme::Option);

}