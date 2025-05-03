"""
The measurements obtained from profiling various region feature splatting implementations are visualised into line charts.
"""
from argparse import ArgumentParser, Namespace
from time import time

from io import BytesIO
from pathlib import Path
import tarfile

from itertools import product
from operator import methodcaller
from typing import NamedTuple, Final

import matplotlib.pyplot as plt
from matplotlib.axes import Axes
from matplotlib.figure import Figure
import numpy as np
import pandas as pd

class _ResultKey(NamedTuple):
	Title: str
	Generator: str
	Splatting: str
	Container: str
	Custom: str
type _Result = dict[_ResultKey, pd.DataFrame]
"""
Search for results using column values from the content as compound key.
"""

class _ResultEnumeration(NamedTuple):
	type ValueCollection = tuple[str, ...]

	Title: ValueCollection
	Generator: ValueCollection
	Splatting: ValueCollection
	Container: ValueCollection
	Custom: ValueCollection
	Colour: ValueCollection
_RESULT_ENUMERATION: Final = _ResultEnumeration(
	Title = ("Radius", "GlobalRegionCount", "LocalRegionCount"),
	Generator = ("Uniform", "Voronoi"),
	Splatting = ("F+", "F-"),
	Container = ("DD", "DS", "SS"),
	Custom = ("Default", "Stress"),
	Colour = ("tomato", "forestgreen", "dodgerblue")
)
"""
All possible values in each column.
"""

def _plotAxis(result: _Result, time_axis: Axes, memory_axe: Axes, title: str, generator: str, splatting: str, custom: str) -> None:
	time_axis.set_ylabel("Time(ms)")
	memory_axe.set_ylabel("Memory(kB)")
	for colour, container in zip(_RESULT_ENUMERATION.Colour, _RESULT_ENUMERATION.Container):
		df = result[_ResultKey(
			Title = title,
			Generator = generator,
			Splatting = splatting,
			Container = container,
			Custom = custom
		)]

		variable = df["variable"]
		time_axis.plot(variable, df["t_median"], color = colour, label = f"{container},Time")
		memory_axe.plot(variable, df["memory"], color = colour, label = f"{container},Memory", linestyle = ":")

def _plotResult(result: _Result, title: str, custom: str, figsize: tuple[int, int],
	generator: _ResultEnumeration.ValueCollection = _RESULT_ENUMERATION.Generator,
	splatting: _ResultEnumeration.ValueCollection = _RESULT_ENUMERATION.Splatting) -> BytesIO:
	fig: Figure
	fig, axe = plt.subplots(len(generator), len(splatting), figsize = figsize,
		squeeze = False, constrained_layout = True)

	for (column, current_splatting), (row, current_generator) in product(*map(enumerate, (splatting, generator))):
		current_axe: Axes = axe[row, column]
		time_axe = current_axe
		memory_axe = current_axe.twinx()

		_plotAxis(result, time_axe, memory_axe, title, current_generator, current_splatting, custom)
		current_axe.set_title(f"{current_generator},{current_splatting}")
		current_axe.grid(visible = True, which = "both", axis = "both")

	# It is alright to use the last axis to to create legend because the drawing order of all axes is the same.
	handle_label: Final = map(methodcaller("get_legend_handles_labels"), (time_axe, memory_axe))
	legend_loc: Final = ("outside upper left", "outside upper right")
	for (handle, label), loc in zip(handle_label, legend_loc):
		fig.legend(handle, label, loc = loc)

	fig_bin: Final = BytesIO()
	fig.savefig(fig_bin, format = "pdf")
	return fig_bin

def addArgument(cmd: ArgumentParser) -> None:
	cmd.add_argument("profiler_result_dir",
		help = "Directory that contains results created and written by the splatting profiler.",
		metavar = "RESULT-DIR"
	)
	cmd.add_argument("profiler_plot_output",
		help = "The tarball where all generated plots are packed.",
		metavar = "PLOT-OUTPUT"
	)

def main(arg: Namespace) -> None:
	result_dir: Final = Path(arg.profiler_result_dir)
	plot_output: Final = Path(arg.profiler_plot_output)

	content: Final = pd.read_csv(result_dir / "Content.csv", index_col = "job id", dtype = {
		"job id" : np.uint32
	})
	result_dtype: Final = {
		"variable" : np.uint16,
		"t_median" : np.float16,
		"memory" : np.uint32
	}
	result: Final[_Result] = {_ResultKey(*others) : pd.read_csv(result_dir / f"{job_id}.csv", dtype = result_dtype)
		for job_id, *others in content.itertuples(index = True)}

	with tarfile.open(plot_output.with_suffix(".tar.xz"), "w|xz", compresslevel = 9) as tar:
		def archive(title: str, custom: str, **kwarg) -> None:
			binary: Final[BytesIO] = _plotResult(result, title, custom, **kwarg)
			binary.seek(0)

			info: Final = tar.tarinfo()
			info.name = str(Path(plot_output.stem) / f"{title}-{custom}.pdf")
			info.size = binary.getbuffer().nbytes
			info.mtime = time()
			info.type = tarfile.REGTYPE

			tar.addfile(info, binary)
		archive("Radius", "Default", figsize = (10, 8))
		archive("GlobalRegionCount", "Default", figsize = (10, 4), generator = ("Voronoi",))
		archive("LocalRegionCount", "Default", figsize = (10, 4), generator = ("Voronoi",))
		archive("Radius", "Stress", figsize = (5, 4), generator = ("Uniform",), splatting = ("F+",))