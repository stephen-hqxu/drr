#include <DisRegRep/Image/Serialisation/Container/Regionfield.hpp>

#include <DisRegRep/Image/Serialisation/Buffer/Tile.hpp>
#include <DisRegRep/Image/Serialisation/Index.hpp>
#include <DisRegRep/Image/Serialisation/Protocol.hpp>
#include <DisRegRep/Image/Tiff.hpp>

#include <DisRegRep/Container/Regionfield.hpp>

#include <DisRegRep/Core/Bit.hpp>
#include <DisRegRep/Core/Exception.hpp>
#include <DisRegRep/Core/MdSpan.hpp>

#include <glm/vector_relational.hpp>
#include <glm/fwd.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <tiff.h>
#include <tiffio.h>

#include <array>
#include <span>

#include <cstdint>

using DisRegRep::Image::Serialisation::Protocol::Implementation, DisRegRep::Container::Regionfield;
using DisRegRep::Core::Bit::BitPerSampleResult, DisRegRep::Core::MdSpan::reverse;

using glm::f32vec2;

using std::to_array, std::span;

namespace {

using Dimension3 = glm::vec<3U, Regionfield::IndexType>;

namespace TiffTag {

constexpr DisRegRep::Image::Tiff::Tag RegionCount = 18007U;

}

}

void Implementation<Regionfield>::initialise() {
	static constexpr auto FieldInfo = to_array<TIFFFieldInfo>({
		{
			.field_tag = TiffTag::RegionCount,
			.field_readcount = 1,
			.field_writecount = 1,
			.field_type = TIFF_BYTE,
			.field_bit = FIELD_CUSTOM,
			.field_oktochange = true,
			.field_name = const_cast<char*>("RegionCount")
		}
	});
	Tiff::defineApplicationTag<FieldInfo.size(), FieldInfo>();
}

void Implementation<Regionfield>::read(const Tiff& tif, Serialisable& regionfield) {
	using ValueType = Serialisable::ValueType;
	using DimensionType = Serialisable::DimensionType;

	DRR_ASSERT(tif.getField<std::uint16_t>(TIFFTAG_ORIENTATION) == ORIENTATION_LEFTTOP);

	DRR_ASSERT(tif.getField<std::uint16_t>(TIFFTAG_SAMPLEFORMAT) == SAMPLEFORMAT_UINT);
	DRR_ASSERT(tif.getField<std::uint16_t>(TIFFTAG_SAMPLESPERPIXEL) == 1U);
	const auto bps_result =
		BitPerSampleResult(BitPerSampleResult::DataTypeTag<ValueType>, tif.getField<std::uint16_t>(TIFFTAG_BITSPERSAMPLE).value());

	DRR_ASSERT(tif.isTiled());

	//Tiles are arranged in left-most rank priority, but regionfield uses layout right.
	//Need to transpose the order of how tiles are written.
	const auto rf_extent = reverse(DimensionType(tif.getImageExtent()));
	regionfield.RegionCount = tif.getField<ValueType>(TiffTag::RegionCount).value();
	regionfield.resize(rf_extent);

	Buffer::Tile<ValueType> tile_buffer;
	tile_buffer.allocate(tif.tileSize());
	const span raw_buffer = tile_buffer.buffer();

	const auto tile_extent = DimensionType(tif.getTileExtent());
	const auto rf_matrix = regionfield.range2d();
	const auto tile_matrix = tile_buffer.shape(decltype(tile_buffer)::EnablePacking, tile_extent, &bps_result);
	for (const auto offset : Index::ForeachTile(rf_extent, tile_extent)) [[likely]] {
		tif.readTile(raw_buffer, Dimension3(reverse(offset), 0U), 0U);
		tile_matrix.toMatrix(rf_matrix, offset);
	}
}

void Implementation<Regionfield>::write(const Tiff& tif, const Serialisable& regionfield, const WriteInfo& write_info) {
	using DimensionType = Serialisable::DimensionType;
	const auto& [compression_scheme, palette_seed] = write_info;

	//It does not make any sense to have only one region, because every element is the same.
	DRR_ASSERT(regionfield.RegionCount > 1U);
	const DimensionType rf_extent = regionfield.extent();
	DRR_ASSERT(glm::all(glm::greaterThan(rf_extent, DimensionType(0U))));
	const BitPerSampleResult bps_result = Core::Bit::minimumBitPerSample(regionfield.span());

	Protocol::setCompressionScheme(tif, compression_scheme);
	tif.setField(TiffTag::RegionCount, regionfield.RegionCount);

	tif.setDefaultMetadata("Regionfield Matrix");
	tif.setField(TIFFTAG_ORIENTATION, ORIENTATION_LEFTTOP);

	tif.setField(TIFFTAG_RESOLUTIONUNIT, RESUNIT_NONE);
	tif.setResolution(f32vec2(1.0F));

	tif.setField(TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
	tif.setField(TIFFTAG_SAMPLESPERPIXEL, 1U);
	tif.setField(TIFFTAG_BITSPERSAMPLE, bps_result.Bit);
	tif.setField(TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

	tif.setField(TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_PALETTE);
	tif.setColourPalette(palette_seed);

	tif.setImageExtent(Dimension3(reverse(rf_extent), 1U));
	tif.setOptimalTileExtent();

	Buffer::Tile<Serialisable::ValueType> tile_buffer;
	tile_buffer.allocate(tif.tileSize());
	const span raw_buffer = tile_buffer.buffer();

	const auto tile_extent = DimensionType(tif.getTileExtent());
	const auto rf_matrix = regionfield.range2d();
	const auto tile_matrix = tile_buffer.shape(decltype(tile_buffer)::EnablePacking, tile_extent, &bps_result);
	for (const auto offset : Index::ForeachTile(rf_extent, tile_extent)) [[likely]] {
		tile_matrix.fromMatrix(rf_matrix, offset);
		tif.writeTile(raw_buffer, Dimension3(reverse(offset), 0U), 0U);
	}
}