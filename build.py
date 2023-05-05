import os
import subprocess
import sys

def get_all_in(path, extensions):
	out_set = set()
	
	for root, dirs, files in os.walk(path):
		for f in files:
			for ext in extensions:
				if f.endswith(ext):
					full_file_path = os.path.join(root, f)
					assert(full_file_path not in out_set)
					out_set.add(full_file_path)
					break
	
	return out_set;

def build_all_except(cc, src_path, exclude, extensions, flags, out):
	actual_excludes = set()
	for excl in exclude:
		actual_excludes.add(os.path.join(src_path, excl))
	exclude = actual_excludes
	
	src_files = get_all_in(src_path, extensions)
	
	args = [cc]
	args += flags
	args.append("-o" + out)
	
	for src_f in src_files:
		if src_f not in exclude:
			args.append(src_f)
	
	return args;


def print_command(command):
	for arg in command:
		print(arg, end=" ")
	print()

def build_release():
	commands = build_all_except(compiler, "src", set(), extensions, ["-DNO_INCLUDE_ASSERTS", "-DNO_TESTS", "-O2"], "out/whippet.out")
	#print_command(commands)
	subprocess.run(commands)
	
def build_debug():
	commands = build_all_except(compiler, "src", set(), extensions, ["-DDEBUG", "-DVERSION_NAME=\"DEBUG BUILD\"", "-Wall", "-Wpedantic", "-g" ], "out/a.out")
	print_command(commands)
	subprocess.run(commands)

def y_n_prompt(msg):
	inp = "";
	while inp.lower() not in ("y", "yes", "n", "no"):
		inp = input(msg + " (y/n): ")
	if inp.lower() in ("y", "yes"):
		return True
	elif inp.lower() in ("n", "no"):
		return False
	assert(False)

def build_custom():
	print(
"""1. Rich terminal interface
	The interactive prompt can run in either basic or rich mode. The rich mode provides more features (line editing, history, highlighting)
	but may potentially be less stable and portable. This setting is simply for what the program should default to, and can be set on startup
	manually by using the --terminal-basic or --terminal-rich arguments.
""")
	rich_term = y_n_prompt("Use rich terminal by default?")
	
	print(
"""2. Manual approval of commands and file handling
	This setting controls whether the interpreter will (by default) ask for permission when attempting to run external programs/commands
	or open files for reading or writing. This makes accidental or unwanted execution of programs or editing of files, either due to 
	user error or bugs in the interpreter, less likely.
	This can manually be selected on startup with the --manual-approve and --no-manual-approve arguments.
""")
	manual_approve = y_n_prompt("Use manual approval by default?")
	
	print(
"""3. Assertions
	Assertions make the code safer and easier to debug, but potentially slower running and more crash-prone (as the program will immedietely close down
	on an assertion failure, instead of potentially missbehaving").
""")
	keep_asserts = y_n_prompt("Keep asserts?")
	
	flags = ["-DNO_TESTS", "-O2"]
	
	if rich_term:
		flags.append("-DSETTING_RICH_TERMINAL=1")
	else:
		flags.append("-DSETTING_RICH_TERMINAL=0")
	
	if manual_approve:
		flags.append("-DSETTING_APPROVE_COMMANDS=1")
	else:
		flags.append("-DSETTING_APPROVE_COMMANDS=0")
	
	if not keep_asserts:
		flags.append("-DNO_INCLUDE_ASSERTS")
	
	commands = build_all_except(compiler, "src", set(), extensions, flags, "out/c_whippet.out")
	print_command(commands);
	subprocess.run(commands);

compiler = "cc"
extensions = [".c"]

if len(sys.argv) == 2 and sys.argv[1] == "d":
	build_debug()
else:
	while True:
		print("Select build option:")
		print("1. Release")
		print("2. Debug")
		print("3. Custom (recommended)")
		print("Q. Quit")
		i = input(":")
		if i == "1":
			build_release()
			break
		elif i == "2":
			build_debug()
			break
		elif i == "3":
			build_custom()
			break
		elif i.lower() in ("q", "quit"):
			break
		
