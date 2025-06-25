"""
To establish a foundation for further analysis, users have the capacity to supply custom region features to be splatted with region feature splatting masks, with a view to generating diverse features.
"""
from argparse import ArgumentParser, Namespace
from time import time

from concurrent.futures import ProcessPoolExecutor

from io import BytesIO
from pathlib import Path
import tarfile

from operator import attrgetter, itemgetter, methodcaller
from typing import Any, Final

from filetype import guess_mime
from PIL import Image
from tifffile import TiffFile

import numpy as np
from numpy.typing import NDArray

from ArgumentGroup import ImageFormat, MultiProcess
import Quantisation

_DEFAULT_COMPUTE_DTYPE: Final = np.float32
type _ImageReadResult = tuple[NDArray[_DEFAULT_COMPUTE_DTYPE], np.dtype[Any]]
"""
Shape [directory, depth, width, height, sample]. Unrelated features are collated into the *directory* axis. Also return the original data type before normalisation is applied.
"""

_TIFF_TAG_IDENTIFIER: Final[int] = 29716
"""
Defined in *Library/DisRegRep/Image/Serialisation/Container/SplattingCoefficient.cpp*.
"""

def _openTiff(filename: Path) -> TiffFile:
	return TiffFile(filename, mode = "r")

def _readImageFromTiff(tif: TiffFile) -> _ImageReadResult:
	series_data: Final = tuple[NDArray[Any]](map(methodcaller("asarray", squeeze = False), tif.pages))
	concatenated: Final = np.concatenate(series_data, axis = 0)
	return Quantisation.normalise(concatenated, _DEFAULT_COMPUTE_DTYPE), concatenated.dtype

def _readImageFromFile(filename: Path) -> _ImageReadResult:
	if guess_mime(filename) == "image/tiff":
		return _readImageFromTiff(_openTiff(filename))

	raw_image: Final = np.array(Image.open(filename, mode = "r"))
	original_dtype: Final = raw_image.dtype
	image: NDArray[_DEFAULT_COMPUTE_DTYPE] = Quantisation.normalise(raw_image, _DEFAULT_COMPUTE_DTYPE)
	del raw_image

	# Attempt to normalise image shape to [width, height, sample].
	image_rank: Final[int] = len(image.shape)
	if image_rank < 2 or image_rank > 3:
		raise ValueError("It is important to note that volumetric image is not supported with image formats other than TIFF.")
	if image_rank == 2:
		image = np.expand_dims(image, axis = -1)
	return np.expand_dims(image, axis = (0, 1)), original_dtype

def addArgument(cmd: ArgumentParser) -> None:
	cmd.add_argument("-F",
		nargs = "+",
		type = Path,
		required = True,
		help = "It is possible to specify features for each region in either a collection of images or a single image with multiple layers.",
		metavar = "FEATURE",
		dest = "masked_feature"
	)
	cmd.add_argument("masked_mask",
		type = Path,
		help = "Dense region feature mask computed by the region feature splatter.",
		metavar = "MASK"
	)
	cmd.add_argument("masked_splat_output",
		type = Path,
		help = "The masks are employed to splat each region feature, subsequently aggregating them to generate outputs which are packed into a tarball.",
		metavar = "SPLAT-OUTPUT"
	)

	ImageFormat.inject(cmd)
	cmd.add_argument("--rescale",
		action = "store_true",
		help = "Rescale the output in such a manner that its values encompass the entire range of its data type.",
		dest = "masked_rescaled"
	)
	MultiProcess.inject(cmd)

def main(arg: Namespace) -> None:
	feature: Final[list[Path]] = arg.masked_feature
	mask: Final[Path] = arg.masked_mask
	splat_output: Final[Path] = arg.masked_splat_output
	image_format_arg: Final[ImageFormat.Argument] = ImageFormat.extract(arg)
	fmt, dtype = image_format_arg
	rescale: Final[bool] = arg.masked_rescaled
	process, = MultiProcess.extract(arg)

	with ProcessPoolExecutor(max_workers = process) as executor:
		feature_read: Final = tuple[_ImageReadResult, ...](executor.map(_readImageFromFile, feature))
	feature_array: Final[NDArray[_DEFAULT_COMPUTE_DTYPE]] = np.swapaxes(
		np.concatenate(tuple(map(itemgetter(0), feature_read)), axis = 0), 0, 1)
	feature_dtype: Final = np.result_type(*map(itemgetter(1), feature_read))
	del feature_read

	mask_tif: Final[TiffFile] = _openTiff(mask)
	mask_array: Final[NDArray[_DEFAULT_COMPUTE_DTYPE]] = _readImageFromTiff(mask_tif)[0]

	splat_array: NDArray[_DEFAULT_COMPUTE_DTYPE] = np.einsum("ij...,ij...->i...", feature_array, mask_array)
	del feature_array, mask_array

	# Pillow is unhappy to have an axis with just one channel.
	if splat_array.shape[-1] == 1:
		splat_array = np.squeeze(splat_array, axis = -1)
	if rescale:
		Quantisation.scale(splat_array, float(np.max(splat_array)), float(np.min(splat_array)), True)
	splat_array_quantised: Final[NDArray[Any]] = Quantisation.unnormalise(splat_array, feature_dtype if dtype is None else dtype)
	del splat_array

	# Images are already compressed, turn compression off to improve IO.
	with tarfile.open(splat_output.with_suffix(".tar"), "w") as tar:
		for identifier, splat in zip(
			map(int.from_bytes,
	   			map(attrgetter("value"),
		   			map(itemgetter(_TIFF_TAG_IDENTIFIER),
						map(attrgetter("tags"), mask_tif.pages)
					)
				)
			),
			np.unstack(splat_array_quantised, axis = 0)
		):
			binary = BytesIO()
			Image.fromarray(splat).save(binary, format = fmt)
			binary.seek(0)

			info = tar.tarinfo()
			info.name = str((Path(splat_output.stem) / f"Identifier-{identifier}").with_suffix(f".{image_format_arg.Extension}"))
			info.size = binary.getbuffer().nbytes
			info.mtime = time()
			info.type = tarfile.REGTYPE

			tar.addfile(info, binary)