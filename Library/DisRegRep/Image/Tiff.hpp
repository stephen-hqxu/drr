#pragma once

#include <tiffio.h>

#include <array>
#include <span>
#include <string_view>
#include <tuple>

#include <memory>

#include <cstdint>

namespace DisRegRep::Image {

/**
 * @brief A handle to a Tagged Image File Format. For all TIFF handle related operations, the behaviour is undefined if the handle is
 * invalid.
 */
class [[nodiscard]] Tiff {
public:

	using Tag = std::uint32_t;

	/**
	 * @brief RGB data, each has size of `1 << bits per sample`.
	 */
	using ConstColourPalette = std::array<std::span<const std::uint16_t>, 3U>;

private:

	struct Close {

		static void operator()(TIFF*) noexcept;

	};
	std::unique_ptr<TIFF, Close> Handle;

	template<typename... Arg>
	void setFieldGeneric(Tag, Arg&&...) const;

	template<typename... Arg>
	[[nodiscard]] std::tuple<Arg...> getFieldGeneric(Tag) const;

public:

	/**
	 * @brief Initialise an empty TIFF handle.
	 */
	constexpr Tiff() noexcept = default;

	/**
	 * @brief Open a TIFF file whose name is *filename* and captures its handle in this instance. All string parameters must be
	 * null-terminated.
	 *
	 * @param filename Name of the TIFF file to be opened.
	 * @param mode Specifies if the file is to be opened for reading, writing, or appending and, optionally, whether to override
	 * certain default aspects of library operation.
	 *
	 * @exception Core::Exception If the open operation fails.
	 */
	Tiff(std::string_view, std::string_view);

	[[nodiscard]] constexpr TIFF* operator*() const noexcept {
		return this->Handle.get();
	}

	[[nodiscard]] explicit constexpr operator bool() const noexcept {
		return static_cast<bool>(this->Handle);
	}

	/**
	 * @brief Set value of a field in the current directory associated with the handle. Overloads are provided to support different tag
	 * value types; notably:
	 * - String: Must be null-terminated.
	 *
	 * @param tag Tag name.
	 * @param value Tag value.
	 */
	void setField(Tag, std::string_view) const;
	void setField(Tag, std::uint16_t) const;

	/**
	 * @brief Set the colour palette of an indexed TIFF.
	 *
	 * @param palette Colour palette data.
	 *
	 * @exception Core::Exception If the handle does not represent an indexed image, or the palette size does not exactly equal to two
	 * to the power of bits per sample.
	 */
	void setColourPalette(const ConstColourPalette&) const;

	/**
	 * @brief Get value of a tag associated with the current directory of the handle. Different getters are provided to support
	 * different tag value types.
	 *
	 * @param tag Tag name.
	 *
	 * @return Tag value.
	 */
	[[nodiscard]] std::uint16_t getFieldUInt16(Tag) const;

	/**
	 * @brief Set various fields of the current TIFF to the project's default metadata.
	 */
	void setDefaultMetadata() const;

};

}