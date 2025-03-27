#pragma once

#include <DisRegRep/Core/Exception.hpp>

#include <glm/fwd.hpp>
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

	using ExtentType = glm::u32vec3; /**< Width, Length, Depth */

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
	 * @brief Set the tile size to the optimal size of the current handle. This only changes the tile width and length.
	 */
	void setOptimalTileSize() const;

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
	 * @brief Get tile extent of the handle. For any axis whose extent is not set, 1 is returned.
	 *
	 * @return Tile extent.
	 */
	[[nodiscard]] ExtentType getTileExtent() const;

	/**
	 * @brief Set various fields of the current TIFF to the project's default metadata.
	 */
	void setDefaultMetadata() const;

};

}