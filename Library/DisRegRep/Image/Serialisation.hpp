#pragma once

namespace DisRegRep::Image {

class Tiff;

/**
 * @brief Define a serialisation protocol for a data structure that allows it to be serialised to an image. The application has freedom
 * controlling the definition of such protocol.
 */
template<typename>
struct Serialisation;

}