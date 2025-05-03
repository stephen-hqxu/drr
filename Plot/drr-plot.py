"""
Furnish a compendium of instrumental applications that facilitate the generation of plots for the purpose of data visualisation, as rendered by the Discrete Region Representation application.
"""
from argparse import ArgumentParser, Namespace
from importlib import import_module

from typing import Final

def _main(arg: Namespace) -> None:
	import_module(arg.plot_engine.title()).main(arg)

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

	for module in ("Masked", "Profiler"):
		engine = import_module(module)
		engine.addArgument(group_plot_engine.add_parser(module.lower(),
			description = engine.__doc__
		))

	_main(parser.parse_args())