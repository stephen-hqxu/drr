#pragma once

#include <DisRegRep/Core/Exception.hpp>

//Check if a LibTIFF API call is successful.
#define DRR_ASSERT_IMAGE_TIFF_ERROR(CODE) DRR_ASSERT((CODE) == 1)