"""
The standardisation of region feature spatting masks is achieved by the division of each splatting algorithm and region into multiple images, with a single directory and depth. This enables the storage of the images in a different format.
"""
from argparse import ArgumentParser, Namespace
from time import time

from concurrent.futures import Future, ProcessPoolExecutor

from io import BytesIO
from pathlib import Path
import tarfile

from operator import methodcaller
from typing import Any, Final, NamedTuple

from PIL import Image
from tifffile import TiffFile

import numpy as np
from numpy.typing import NDArray

from ArgumentGroup import ImageFormat, MultiProcess
import Quantisation

type _MaskArray = NDArray[np.uint16]

class _ImageSplitFuture(NamedTuple):
	"""
	An entry of a collection of images split from a mask.
	"""
	Directory: int
	"""
	Splatting algorithm index.
	"""
	Depth: int
	"""
	Region identifier.
	"""
	Binary: Future[BytesIO]
	"""
	Raw image binary data.
	"""

def _saveMask(mask: NDArray[np.uint16], need_conversion: bool, target_dtype: np.dtype[Any] | None, fmt: str) -> BytesIO:
	# [1, 1, height, width] -> [1, height, width] -> [height, width]
	mask_converted: NDArray[Any] = np.squeeze(np.squeeze(mask, axis = 0), axis = 0)
	if need_conversion:
		assert target_dtype is not None
		mask_converted = Quantisation.unnormalise(Quantisation.normalise(mask_converted, np.float32), target_dtype) # type: ignore

	binary = BytesIO()
	image: Final = Image.fromarray(mask_converted)
	image.save(binary, format = fmt)
	return binary

def _splitMask(executor: ProcessPoolExecutor, mask: _MaskArray, image_format_arg: ImageFormat.Argument) -> list[_ImageSplitFuture]:
	fmt, dtype = image_format_arg
	need_conversion: Final[bool] = False if dtype is None else dtype != mask.dtype

	future_image_buffer: Final[list[_ImageSplitFuture]] = []
	# [directory, depth, height, width]
	for directory, mask_directory in enumerate(np.split(mask, mask.shape[0], axis = 0)):
		# [1, depth, height, width]
		for depth, mask_depth in enumerate(np.split(mask_directory, mask_directory.shape[1], axis = 1)):
			# [1, 1, height, width]
			future_image_buffer.append(_ImageSplitFuture(
				Directory = directory,
				Depth = depth,
				Binary = executor.submit(_saveMask, mask_depth, need_conversion, dtype, fmt)
			))
	return future_image_buffer

def addArgument(cmd: ArgumentParser) -> None:
	cmd.add_argument("split_mask",
		nargs = "+",
		type = Path,
		help = "Dense region feature mask is to be separated into individual images. It is possible to specify multiple masks in order to harness hardware parallelisation to enhance efficiency.",
		metavar = "MASK",
	)
	cmd.add_argument("-o", "--output-dir",
		type = Path,
		required = True,
		help = "Split images are stored into a tarball in the specified directory. The filename of each is derived from each of the provided masks.",
		metavar = "OUTPUT-DIR",
		dest = "split_output_dir"
	)

	ImageFormat.inject(cmd)
	MultiProcess.inject(cmd)

def main(arg: Namespace) -> None:
	mask: Final[list[Path]] = arg.split_mask
	output_dir: Final[Path] = arg.split_output_dir
	image_format_arg: Final[ImageFormat.Argument] = ImageFormat.extract(arg)
	process, = MultiProcess.extract(arg)

	with ProcessPoolExecutor(max_workers = process) as executor:
		future_image_buffer: Final[list[list[_ImageSplitFuture]]] = []
		for mask_filename in mask:
			tif = TiffFile(mask_filename, mode = "r")
			# [directory, depth, height, width, sample] where *sample* is always 1.
			mask_array: NDArray[np.uint16] = np.squeeze(
				np.concatenate(tuple(map(methodcaller("asarray", squeeze = False), tif.pages))), axis = 4)
			assert mask_array.dtype == np.uint16

			future_image_buffer.append(_splitMask(executor, mask_array, image_format_arg))

		for mask_filename, mask_buffer in zip(mask, future_image_buffer):
			mask_filename_stem: str = mask_filename.stem
			with tarfile.open((output_dir / mask_filename_stem).with_suffix(".tar"), mode = "w") as tar:
				for directory, depth, binary_future in mask_buffer:
					binary: BytesIO = binary_future.result()
					binary.seek(0)

					info = tar.tarinfo()
					info.name = str((Path(mask_filename_stem) / f"Directory-{directory}_Depth-{depth}").with_suffix(f".{image_format_arg.Extension}"))
					info.size = binary.getbuffer().nbytes
					info.mtime = time()
					info.type = tarfile.REGTYPE

					tar.addfile(info, binary)