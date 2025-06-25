"""
It is possible to stipulate the number of processes that will be executed concurrently to accomplish a specific task.
"""
from argparse import ArgumentParser, Namespace
from typing import NamedTuple

class Argument(NamedTuple):
	"""
	Multi process arguments.
	"""
	Process: int

def inject(cmd: ArgumentParser) -> None:
	cmd.add_argument("-p",
		default = 1,
		type = int,
		help = "The number of processes to run the task in parallel may be specified.",
		metavar = "PROCESS",
		dest = "argument_group_multi_process_process"
	)

def extract(arg: Namespace) -> Argument:
	return Argument(
		Process = arg.argument_group_multi_process_process
	)