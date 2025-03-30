#include <DisRegRep/Image/Serialisation/Container/Regionfield.hpp>

#include <DisRegRep/Image/Serialisation/Buffer/Tile.hpp>
#include <DisRegRep/Image/Serialisation/Bit.hpp>
#include <DisRegRep/Image/Serialisation/Index.hpp>
#include <DisRegRep/Image/Serialisation/Protocol.hpp>
#include <DisRegRep/Image/Tiff.hpp>

#include <DisRegRep/Container/Regionfield.hpp>

#include <DisRegRep/Core/Exception.hpp>

#include <glm/vector_relational.hpp>
#include <glm/fwd.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <tiff.h>

#include <span>
#include <tuple>

using DisRegRep::Image::Serialisation::Protocol, DisRegRep::Container::Regionfield;

using glm::f32vec2;

using std::span, std::make_from_tuple;

void Protocol<Regionfield>::write(const Tiff& tif, const Serialisable& regionfield, const Tiff::ColourPaletteRandomEngineSeed seed) {
	using Dimension3 = glm::vec<3U, Serialisable::IndexType>;

	//It does not make any sense to have only one region, because every element is the same.
	DRR_ASSERT(regionfield.RegionCount > 1U);
	const Serialisable::DimensionType rf_extent = regionfield.extent();
	DRR_ASSERT(glm::all(glm::greaterThan(rf_extent, Serialisable::DimensionType(0U))));
	const Bit::BitPerSampleResult bps_result = Bit::minimumBitPerSample(regionfield.span());

	tif.setDefaultMetadata("Regionfield Matrix");
	tif.setField(TIFFTAG_ORIENTATION, ORIENTATION_LEFTTOP);

	tif.setField(TIFFTAG_RESOLUTIONUNIT, RESUNIT_NONE);
	tif.setResolution(f32vec2(1.0F));

	tif.setField(TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
	tif.setField(TIFFTAG_SAMPLESPERPIXEL, 1U);
	tif.setField(TIFFTAG_BITSPERSAMPLE, bps_result.Bit);
	tif.setField(TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

	tif.setField(TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_PALETTE);
	tif.setColourPalette(seed);

	tif.setImageExtent(Dimension3(rf_extent, 1U));
	tif.setOptimalTileSize();

	Buffer::Tile<Serialisable::ValueType> tile_buffer;
	tile_buffer.allocate(tif.tileSize());
	const span raw_buffer = tile_buffer.buffer();

	const auto tile_extent = static_cast<Serialisable::DimensionType>(tif.getTileExtent());
	const auto rf_matrix = regionfield.range2d();
	const auto tile_matrix = tile_buffer.shape<true>(tile_extent, &bps_result);
	for (const auto offset : Index::ForeachTile(rf_extent, tile_extent)) [[likely]] {
		const auto [offset_x, offset_y] = offset;
		tile_matrix.fromMatrix(rf_matrix, make_from_tuple<Serialisable::DimensionType>(offset));
		//Tiles are arranged in left-most rank priority, but regionfield uses layout right.
		//Need to transpose the order of how tiles are written.
		tif.writeTile(raw_buffer, Dimension3(offset_y, offset_x, 0U), 0U);
	}
}