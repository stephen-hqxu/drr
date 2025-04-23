"""
Furnish a compendium of instrumental applications that facilitate the generation of plots for the purpose of data visualisation, as rendered by the Discrete Region Representation application.
"""
from argparse import ArgumentParser, Namespace

from typing import assert_never, Final

import ProfilerResult

def _main(arg: Namespace) -> None:
	match arg.plot_engine:
		case "profiler": ProfilerResult.main(arg)
		case _: assert_never(arg.plot_engine)

if __name__ == "__main__":
	parser: Final = ArgumentParser(
		description = __doc__,
		epilog = "For further details, please refer to the Discrete Region Representation main programme command line documentation.",
		allow_abbrev = False
	)
	group_plot_engine: Final = parser.add_subparsers(
		title = "plot-engine",
		description = "A collection of engines is implemented, with each engine specialised for the creation of plots for a specific category of data.",
		dest = "plot_engine",
		required = True
	)

	for engine in (ProfilerResult,):
		engine.addArgument(group_plot_engine.add_parser("profiler",
			description = engine.__doc__
		))

	_main(parser.parse_args())