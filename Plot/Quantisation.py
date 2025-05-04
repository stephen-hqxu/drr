"""
Map input values from a large set to output values in a small set.
"""
from typing import Final

import numpy as np
from numpy.typing import NDArray

def scale[DType: np.floating](array: NDArray[DType], upper: float, lower: float, inplace: bool = False) -> NDArray[DType]:
	"""
	Scale numbers in an array.

	:param array: Array to be scaled.
	:param upper: The new upper bound.
	:param lower: The new lower bound.
	:param inplace: If scaling is performed directly to `array`, or a copy is made.

	:return output: Array scaled from `array`.
	"""
	if not np.isdtype(array.dtype, "real floating"):
		raise TypeError("It is only possible to scale an array of floats.")

	output = array if inplace else array.copy()
	output -= lower
	output /= upper - lower
	return output

def normalise[Target: np.floating](array: NDArray[np.integer], target_dtype: type[Target]) -> NDArray[Target]:
	"""
	Normalise numbers in an array.

	:param array: Range [INT_N_MIN, INT_N_MAX].
	:param target_dtype: Data type of the output array.

	:return output: Range [0.0, 1.0].
	"""
	dtype: Final = array.dtype
	if not np.isdtype(dtype, "integral"):
		raise TypeError("It is only possible to normalise an array of integers.")

	limit: Final = np.iinfo(dtype)
	return scale(array.astype(target_dtype), limit.max, limit.min, True)

def unnormalise[Target: np.integer](array: NDArray[np.floating], target_dtype: type[Target]) -> NDArray[Target]:
	"""
	Unnormalise numbers in an array.

	:param array: Range [0.0, 1.0].
	:param target_dtype: Data type of the output array.

	:return output: Range [INT_N_MIN, INT_N_MAX].
	"""
	if not np.isdtype(target_dtype, "integral"):
		raise TypeError("It is only possible to unnormalise to an array of integers.")

	limit: Final = np.iinfo(target_dtype)
	output: NDArray[np.floating] = array.copy()
	output *= limit.max - limit.min
	output -= limit.min
	return np.round(output, out = output).astype(target_dtype)