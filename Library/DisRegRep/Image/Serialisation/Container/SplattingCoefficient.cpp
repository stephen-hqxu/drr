#include <DisRegRep/Image/Serialisation/Container/SplattingCoefficient.hpp>

#include <DisRegRep/Image/Serialisation/Buffer/Tile.hpp>
#include <DisRegRep/Image/Serialisation/Index.hpp>
#include <DisRegRep/Image/Serialisation/Protocol.hpp>
#include <DisRegRep/Image/Tiff.hpp>

#include <DisRegRep/Container/SplattingCoefficient.hpp>

#include <DisRegRep/Core/Exception.hpp>
#include <DisRegRep/Core/MdSpan.hpp>

#include <glm/vector_relational.hpp>
#include <glm/fwd.hpp>
#include <glm/vec2.hpp>

#include <tiff.h>
#include <tiffio.h>

#include <array>
#include <span>

#include <algorithm>
#include <functional>
#include <ranges>

#include <concepts>
#include <limits>
#include <type_traits>

#include <cmath>

using DisRegRep::Image::Serialisation::Protocol, DisRegRep::Container::SplattingCoefficient::DenseMask;
using DisRegRep::Core::MdSpan::reverse;

using glm::f32vec2;

using std::to_array, std::span,
	std::ranges::for_each,
	std::bind_back, std::bit_or,
	std::views::transform, std::views::zip;
using std::unsigned_integral, std::remove_reference_t;

namespace {

namespace TiffTag {

constexpr DisRegRep::Image::Tiff::Tag Identifier = 29716U;

}

template<unsigned_integral PixelType>
void write(const DisRegRep::Image::Tiff& tif, const DenseMask& dense_mask, const unsigned_integral auto identifier,
	DisRegRep::Image::Serialisation::Buffer::Tile<PixelType>& tile_buffer) {
	using Dimension2Type = DenseMask::Dimension2Type;
	using Dimension3Type = DenseMask::Dimension3Type;
	using PixelLimit = std::numeric_limits<PixelType>;

	const Dimension3Type mask_extent = dense_mask.extent();
	DRR_ASSERT(glm::all(glm::greaterThan(mask_extent, Dimension3Type(0U))));

	tif.setField(TiffTag::Identifier, identifier);

	tif.setDefaultMetadata("Dense Region Feature Splatting Mask");
	tif.setField(TIFFTAG_ORIENTATION, ORIENTATION_LEFTTOP);

	tif.setField(TIFFTAG_RESOLUTIONUNIT, RESUNIT_NONE);
	tif.setResolution(f32vec2(1.0F));

	//Since mask values are all unsigned normalised (i.e. [0.0, 1.0]), we can convert it to a fixed point representation.
	tif.setField(TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
	tif.setField(TIFFTAG_SAMPLESPERPIXEL, 1U);
	tif.setField(TIFFTAG_BITSPERSAMPLE, PixelLimit::digits);
	tif.setField(TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

	tif.setField(TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);

	tif.setImageExtent(Dimension3Type(reverse(Dimension2Type(mask_extent)), mask_extent.z));
	//Although it is a 3D matrix, we reckon the region axis is shallow compared to width and height axis.
	//It does not worth to use a 3D tile because the region axis may have a lot of paddings.
	tif.setOptimalTileExtent();

	tile_buffer.allocate(tif.tileSize());
	const span raw_buffer = tile_buffer.buffer();

	const Dimension3Type tile_extent = tif.getTileExtent();
	const auto mask_matrix = dense_mask.range2d();
	const auto tile_matrix = tile_buffer.shape(remove_reference_t<decltype(tile_buffer)>::DisablePacking, Dimension2Type(tile_extent));
	for (const auto offset : DisRegRep::Image::Serialisation::Index::ForeachTile(mask_extent, tile_extent)) [[likely]] {
		const Dimension2Type offset_xy = offset;
		tile_matrix.fromMatrix(
			mask_matrix | transform(bind_back(bit_or {}, transform([region = offset.z](const auto proxy) constexpr noexcept -> PixelType {
				return std::round((*proxy)[region] * PixelLimit::max());
			}))),
			offset_xy
		);
		//Remember to transpose the tile writing order.
		tif.writeTile(raw_buffer, Dimension3Type(reverse(offset_xy), offset.z), 0U);
	}
}

}

void Protocol<DenseMask>::initialise() {
	static constexpr auto FieldInfo = to_array<TIFFFieldInfo>({
		{
			.field_tag = TiffTag::Identifier,
			.field_readcount = 1,
			.field_writecount = 1,
			.field_type = TIFF_BYTE,
			.field_bit = FIELD_CUSTOM,
			.field_oktochange = true,
			.field_name = const_cast<char*>("Identifier")
		}
	});
	Tiff::defineApplicationTag<FieldInfo.size(), FieldInfo>();
}

void Protocol<DenseMask>::write(const Tiff& tif, const Serialisable& dense_mask, const IdentifierType identifier) {
	Buffer::Tile<PixelType> tile_buffer;
	::write(tif, dense_mask, identifier, tile_buffer);
}

void Protocol<DenseMask>::write(
	const Tiff& tif, const span<const Serialisable* const> dense_mask, const span<const IdentifierType> identifier) {
	Buffer::Tile<PixelType> tile_buffer;
	for_each(zip(dense_mask, identifier), [&](const auto mask_id) {
		const auto [mask, id] = mask_id;
		::write(tif, *mask, id, tile_buffer);
		tif.writeDirectory();
	});
}