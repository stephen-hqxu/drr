#pragma once

#include <DisRegRep/Core/Exception.hpp>

#include <glm/fwd.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <tiffio.h>

#include <array>
#include <span>

#include <optional>
#include <string_view>
#include <tuple>

#include <memory>
#include <random>
#include <utility>

#include <type_traits>

#include <cassert>
#include <cstddef>
#include <cstdint>

namespace DisRegRep::Image {

/**
 * @brief A handle to a Tagged Image File Format. For all TIFF handle related operations, the behaviour is undefined if the handle is
 * invalid.
 */
class [[nodiscard]] Tiff {
public:

	using Tag = std::uint32_t;

	using ColourPaletteElement = std::uint16_t;
	/**
	 * @brief RGB data, each has size of `1 << bits per sample`.
	 */
	using ConstColourPalette = std::array<std::span<const ColourPaletteElement>, 3U>;
	using ColourPaletteRandomEngine = std::ranlux48;
	using ColourPaletteRandomEngineSeed = ColourPaletteRandomEngine::result_type;

	using BinaryBuffer = std::span<std::byte>;

private:

	struct Close {

		static void operator()(TIFF*) noexcept;

	};
	std::unique_ptr<TIFF, Close> Handle;

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
	 * @brief Release all resources and close the handle.
	 */
	void close() noexcept {
		this->Handle.reset();
	}

	/**
	 * @brief Set value of a field in the current directory associated with the handle.
	 *
	 * @tparam Value Tag value type.
	 *
	 * @param tag Tag name.
	 * @param value Tag value.
	 */
	template<typename... Value>
	requires(sizeof...(Value) > 0UZ)
	void setField(const Tag tag, Value&&... value) const {
		assert(*this);
		//NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
		DRR_ASSERT(TIFFSetField(**this, tag, std::forward<Value>(value)...));
	}

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
	 * @brief Generate a random colour palette of an indexed TIFF.
	 *
	 * @param seed Random seed.
	 *
	 * @exception Core::Exception If the handle does not represent an indexed image.
	 */
	void setColourPalette(ColourPaletteRandomEngineSeed) const;

	/**
	 * @brief Set image width, length and depth.
	 *
	 * @param extent Image extent to be set to.
	 */
	void setImageExtent(glm::u32vec3) const;

	/**
	 * @brief Set the number of pixels per resolution unit in image width and length directions.
	 *
	 * @param resolution Pixel per resolution unit.
	 */
	void setResolution(glm::f32vec2) const;

	/**
	 * @brief Set the tile extent to the optimal extent of the current handle. This only changes the tile width and length.
	 */
	void setOptimalTileExtent() const;

	/**
	 * @brief Get value of a tag associated with the current directory of the handle.
	 *
	 * @tparam Value Tag value type.
	 *
	 * @param tag Tag name.
	 *
	 * @return Optional tag value depends on if the tag is defined in the current directory. It is an optional scalar value if `Value`
	 * pack size is one, or an optional of tuple otherwise.
	 */
	template<typename... Value, typename ValueTup = std::tuple<Value...>>
	requires(sizeof...(Value) > 0UZ)
	[[nodiscard]] std::optional<std::conditional_t<
		sizeof...(Value) == 1U,
		std::tuple_element_t<0U, ValueTup>,
		ValueTup
	>> getField(const Tag tag) const noexcept {
		assert(*this);

		ValueTup value;
		//NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
		if (std::apply([tif = **this, tag](auto&... v) noexcept { return TIFFGetField(tif, tag, std::addressof(v)...); }, value)
			== 0) [[unlikely]] {
			return std::nullopt;
		}
		if constexpr (sizeof...(Value) == 1U) {
			return std::get<0U>(std::move(value));
		} else {
			return value;
		}
	}

	/**
	 * @brief Get the expected size of **each channel** of the colour palette.
	 *
	 * @return Colour palette size per channel.
	 */
	[[nodiscard]] std::size_t getColourPaletteSize() const;

	/**
	 * @brief Get image width, length and depth.
	 *
	 * @return Image extent of this handle.
	 */
	[[nodiscard]] glm::u32vec3 getImageExtent() const;

	/**
	 * @brief Get tile extent of the handle. For any axis whose extent is not set, 1 is returned.
	 *
	 * @return Tile extent.
	 */
	[[nodiscard]] glm::u32vec3 getTileExtent() const;

	/**
	 * @brief Set various fields of the current TIFF to the project's default metadata.
	 *
	 * @param description Describe the subject of the image. Must be null-terminated.
	 */
	void setDefaultMetadata(std::string_view) const;

	/**
	 * @brief Check if image data has a tiled organisation.
	 *
	 * @return True if image is tiled.
	 */
	[[nodiscard]] bool isTiled() const noexcept;

	/**
	 * @brief Equivalent size for a tile of data as it would be read or written.
	 *
	 * @note Please use the 64-bit size version for a BigTIFF handle. This is for the 32-bit version only.
	 *
	 * @return Tile size in bytes.
	 *
	 * @exception Core::Exception If an error occur.
	 */
	[[nodiscard]] std::size_t tileSize() const;

	/**
	 * @brief Return the data for the tile containing the specified coordinates.
	 *
	 * @param buffer Decompressed data stored to, written in native byte- and bit-ordering.
	 * @param coordinate Tile coordiante.
	 * @param sample Sample index when data are organised in separate planes.
	 */
	void readTile(BinaryBuffer, glm::u32vec3, std::uint16_t) const;

	/**
	 * @brief Write the data for the tile containing the specified coordinates.
	 *
	 * @param buffer Data to be written.
	 * @param coordiante Tile coordinate.
	 * @param sample Sample index when data are organised in separate planes.
	 *
	 * @exception Core::Exception If an error is detected.
	 */
	void writeTile(BinaryBuffer, glm::u32vec3, std::uint16_t) const;

	/**
	 * @brief Define an application tag so that it can be read or written.
	 *
	 * @tparam Size Number of field info.
	 * @tparam FieldInfo An array of field info structures. Each provides a definition for a tag.
	 *
	 * @exception Core::Exception If application tags are redefined. This function does not check the content of the field info array,
	 * however. Providing duplicate entries is an error that will not be captured.
	 */
	template<std::size_t Size, const std::array<TIFFFieldInfo, Size>& FieldInfo>
	static void defineApplicationTag() {
		static constinit bool defined = false;
		static constinit TIFFExtendProc parent_extender = nullptr;
		DRR_ASSERT(!defined);

		constexpr auto extender = [](TIFF* const tif) static noexcept -> void {
			TIFFMergeFieldInfo(tif, FieldInfo.data(), Size);
			if (parent_extender) [[likely]] {
				parent_extender(tif);
			}
		};
		parent_extender = TIFFSetTagExtender(extender);
		defined = true;
	}

};

}