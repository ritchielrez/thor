#!/usr/bin/env python3

import sys
import subprocess
import platform
import shutil

target = "thor"
cc = ""
src_file = "./src/main.c"
out = "main"
cflags: list[str] = []
cmd: list[str] = []

def shift_args(argv: list[str]) -> str:
	# argv is here a reference to a list.
	# Use slice assignment to change the actual list being referred to, instead of changing what it is referring to
	result = argv[0]
	argv[:] = argv[1:]
	return result

def com() :
	if platform.system() == "Windows":
		if shutil.which("cl") is not None:
			cc = "cl.exe"
			cflags = ["-EHsc", "-nologo", "-W4", "-Zi", "-MTd", "-DDEBUG", "-fsanitize=address", "-std:clatest", f"-Fe{out}.exe"]
		elif shutil.which("gcc") is not None:
			cc = "gcc.exe"
			cflags = ["-g", "-Wall", "-Wextra", "-o", out]
		else:
			print("Error: cl.exe or gcc.exe not found")
			sys.exit(1)
	else:
		print("Only Windows is supported for now")
		sys.exit(1)

	cmd.append(cc)
	[cmd.append(cflag) for cflag in cflags]
	cmd.append(src_file)
	print("Compiling:", cmd)
	subprocess.run(cmd)

def run():
	com()

def print_usage(prg_name: str) -> None:
	print(f"Usage: {prg_name} <subcommand> [arg]")
	print("subcommands:")
	print("    com     Compiles", target)
	print("    run     Compiles and runs", target)
	print("    help    Print this help usage information")

def main() -> int:
	argv = sys.argv
	prg_name = shift_args(argv)

	if(len(argv) == 0):
		print_usage(prg_name)
		print("Error: no subcommand provided")
		return 1

	subcommand = shift_args(argv)	
	if subcommand == "help":
		print_usage(prg_name)
	elif subcommand == "com":
		com()
	elif subcommand == "run":
		run()
	else:
		print_usage(prg_name)
		print("Error: unknown subcommand", subcommand)
		return 1

	return 0

if __name__ == "__main__":
	ret_code = main()
	sys.exit(ret_code)