"""
The standardisation of region feature spatting masks is achieved by the division of each splatting algorithm and region into multiple images, with a single directory and depth. This enables the storage of the images in a different format.
"""
from argparse import ArgumentParser, Namespace

from concurrent.futures import Future, ProcessPoolExecutor

from io import BytesIO
from pathlib import Path
import tarfile
from time import time

from typing import Any, Final, NamedTuple

from PIL import Image
import tifffile

import numpy as np
from numpy.typing import NDArray

from ArgumentGroup import ImageFormat
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

def _saveImage(data: NDArray[Any], fmt: str) -> BytesIO:
	binary = BytesIO()
	image: Final = Image.fromarray(data)
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
			# [1, 1, height, width] -> [1, height, width] -> [height, width]
			mask_depth = np.squeeze(np.squeeze(mask_depth, axis = 0), axis = 0)

			mask_converted: NDArray[Any]
			if need_conversion:
				assert dtype is not None
				mask_converted = Quantisation.unnormalise(Quantisation.normalise(mask_depth, np.float32), dtype)
			else:
				mask_converted = mask_depth

			future_image_buffer.append(_ImageSplitFuture(
				Directory = directory,
				Depth = depth,
				Binary = executor.submit(_saveImage, mask_converted, fmt)
			))
	return future_image_buffer

def addArgument(cmd: ArgumentParser) -> None:
	cmd.add_argument("split_mask",
		nargs = "+",
		type = Path,
		help = "Dense region feature mask is to be separated into individual images. It is possible to specify multiple masks in order to harness hardware parallelisation to enhance efficiency.",
		metavar = "MASK",
	)
	cmd.add_argument("-o,--output-dir",
		type = Path,
		required = True,
		help = "Split images are stored into a tarball in the specified directory. The filename of each is derived from each of the provided masks.",
		metavar = "OUTPUT-DIR",
		dest = "split_output_dir"
	)

	ImageFormat.inject(cmd)
	cmd.add_argument("-p",
		default = 1,
		type = int,
		help = "The number of processes to run the mask splitting task in parallel may be specified.",
		metavar = "PROCESS",
		dest = "split_process"
	)

def main(arg: Namespace) -> None:
	mask: Final[list[Path]] = arg.split_mask
	output_dir: Final[Path] = arg.split_output_dir
	image_format_arg: Final[ImageFormat.Argument] = ImageFormat.extract(arg)
	process: Final[int] = arg.split_process

	with ProcessPoolExecutor(max_workers = process) as executor:
		future_image_buffer: Final[list[list[_ImageSplitFuture]]] = []
		for mask_filename in mask:
			mask_array: NDArray[np.uint16] = tifffile.imread(mask_filename, mode = "r")
			assert mask_array.dtype == np.uint16

			future_image_buffer.append(_splitMask(executor, mask_array, image_format_arg))

		for mask_filename, mask_buffer in zip(mask, future_image_buffer):
			with tarfile.open(output_dir / mask_filename.with_suffix(".tar"), mode = "w") as tar:
				for directory, depth, binary_future in mask_buffer:
					binary: BytesIO = binary_future.result()
					binary.seek(0)

					info = tar.tarinfo()
					info.name = str((Path(mask_filename.stem) / f"Directory-{directory}_Depth-{depth}").with_suffix(f".{image_format_arg.Extension}"))
					info.size = binary.getbuffer().nbytes
					info.mtime = time()
					info.type = tarfile.REGTYPE

					tar.addfile(info, binary)