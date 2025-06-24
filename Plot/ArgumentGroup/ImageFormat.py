"""
The user is able to customise the output format for an image file.
"""
from argparse import ArgumentParser, Namespace
from typing import Any, NamedTuple

import numpy as np

class Argument(NamedTuple):
	"""
	Image format arguments.
	"""
	Format: str
	ElementDType: np.dtype[Any] | None

	@property
	def Extension(self) -> str:
		"""
		Deduced standard extension of an image format.
		"""
		match(self.Format.lower()):
			case "jpeg": return "jpg"
			case "tiff": return "tif"
			case _: return self.Format

def inject(cmd: ArgumentParser) -> None:
	cmd.add_argument("--format",
		required = True,
		help = "Output image format.",
		metavar = "FORMAT",
		dest = "argument_group_image_format_format"
	)
	cmd.add_argument("--element-dtype",
		help = "Output element data type. In the absence of specification, this value is deduced from the input data.",
		metavar = "ELEMENT-DTYPE",
		dest = "argument_group_image_format_element_dtype"
	)

def extract(arg: Namespace) -> Argument:
	return Argument(
		Format = arg.argument_group_image_format_format,
		ElementDType = None if arg.argument_group_image_format_element_dtype is None else np.dtype(arg.argument_group_image_format_element_dtype)
	)