from distutils.dir_util import copy_tree
from os import environ, getenv, getcwd, path

from SCons.Script import ARGUMENTS


# def get_active_branch_name():
#     from os import getcwd, path

#     head_dir = path.join(getcwd(), ".git", "HEAD")
#     with open(head_dir, "r") as f: content = f.read().splitlines()

#     for line in content:
#         if line[0:4] == "ref:":
#             return line.partition("refs/heads/")[2]


# if not getenv("SCONS_CACHE_LIMIT"):
#     environ["SCONS_CACHE_LIMIT"] = "5120"


# if not getenv("SCONS_CACHE"):
#     base_cache_path = path.join(getcwd(), ".scons_cache")
#     environ["SCONS_CACHE"] = base_cache_path
#     branch_name = get_active_branch_name()
#     if branch_name != "master":
#         cache_path = path.join(getcwd(), f".scons_cache__{branch_name}")
#         if (not path.isdir(cache_path)) and (path.isdir(base_cache_path)):
#             copy_tree(base_cache_path, cache_path)
#         environ["SCONS_CACHE"] = cache_path


compiledb = ARGUMENTS.get("compiledb", "yes")
platform = ARGUMENTS.get("platform", "linux")
dev_build = ARGUMENTS.get("dev_build", "yes")
debug_symbols = ARGUMENTS.get("debug_symbols", "yes")
dev_mode = ARGUMENTS.get("dev_mode", "yes")
scu_build = ARGUMENTS.get("scu_build", "yes")
optimize = ARGUMENTS.get("optimize", "none") 
fast_unsafe = ARGUMENTS.get("fast_unsafe", "yes")


if ARGUMENTS.get("platform", "linux") == "linux":
    use_llvm = ARGUMENTS.get("use_llvm", "yes") 
    linker = ARGUMENTS.get("linker", "mold") 
