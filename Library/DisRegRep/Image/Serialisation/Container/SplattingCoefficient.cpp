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

#include <span>
#include <tuple>

#include <functional>
#include <ranges>

#include <utility>

#include <limits>

#include <cmath>

using DisRegRep::Image::Serialisation::Protocol, DisRegRep::Container::SplattingCoefficient::DenseMask;

using glm::f32vec2;

using std::span, std::make_from_tuple,
	std::bind_back, std::bit_or,
	std::views::transform;

void Protocol<DenseMask>::write(const Tiff& tif, const Serialisable& dense_mask) {
	using PixelLimit = std::numeric_limits<PixelType>;
	const Serialisable::Dimension3Type mask_extent = Core::MdSpan::toVector(dense_mask.mapping().extents());
	DRR_ASSERT(glm::all(glm::greaterThan(mask_extent, Serialisable::Dimension3Type(0U))));

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

	tif.setImageExtent(mask_extent);
	//Although it is a 3D matrix, we reckon the region axis is shallow compared to width and height axis.
	//It does not worth to use a 3D tile because the region axis may have a lot of paddings.
	tif.setOptimalTileExtent();

	Buffer::Tile<PixelType> tile_buffer;
	tile_buffer.allocate(tif.tileSize());
	const span raw_buffer = tile_buffer.buffer();

	const Serialisable::Dimension3Type tile_extent = tif.getTileExtent();
	const auto mask_matrix = dense_mask.range2d() | transform(bind_back(bit_or {}, transform([](auto proxy) static constexpr noexcept {
		return *std::move(proxy)
			| transform([](const auto mask) static constexpr noexcept -> PixelType { return std::round(mask * PixelLimit::max()); });
	})));
	const auto tile_matrix = tile_buffer.shape(decltype(tile_buffer)::DisablePacking, tile_extent);
	for (const auto offset : Index::ForeachTile(mask_extent, tile_extent)) [[likely]] {
		const auto [offset_x, offset_y, offset_region] = offset;
		tile_matrix.fromMatrix(mask_matrix, make_from_tuple<Serialisable::Dimension3Type>(offset));
		//Remember to transpose the tile writing order.
		tif.writeTile(raw_buffer, Serialisable::Dimension3Type(offset_y, offset_x, offset_region), 0U);
	}
}