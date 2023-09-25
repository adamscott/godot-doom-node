#!/usr/bin/env python
import os
import sys
import shutil
from SCons.Errors import UserError

def normalize_path(val):
    return val if os.path.isabs(val) else os.path.join(env.Dir("#").abspath, val)


def validate_compiledb_file(key, val, env):
    if not os.path.isdir(os.path.dirname(normalize_path(val))):
        raise UserError("Directory ('%s') does not exist: %s" % (key, os.path.dirname(val)))


def get_compiledb_file(env):
    return normalize_path(env.get("compiledb_file", "compile_commands.json"))

env = Environment()
customs = ["custom.py"]
opts = Variables(customs, ARGUMENTS)
opts.Add(BoolVariable("compiledb", "Generate compilation DB (`compile_commands.json`) for external tools", False))
opts.Add(
    PathVariable(
        "compiledb_file",
        help="Path to a custom compile_commands.json",
        default=normalize_path("compile_commands.json"),
        validator=validate_compiledb_file,
    )
)
opts.Update(env)

clonedEnv = env.Clone()
clonedEnv["compiledb"] = False
env = SConscript("godot-cpp/SConstruct", {
    "env": clonedEnv
})

# For reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags

# tweak this if you want to use different folders, or more folders, to store your source code in.
spawn_executable_name = f"godot-doom-node-spawn{env['suffix']}{env['PROGSUFFIX']}"

env.Append(CPPPATH=[os.path.abspath("src/"), os.path.abspath("thirdparty/doomgeneric")])
env.Append(CPPDEFINES=["FEATURE_SOUND_GODOT", f"SPAWN_EXECUTABLE_NAME=\"\\\"{spawn_executable_name}\\\"\""])

sources = Glob("src/*.cpp")
spawn_sources = Glob("src/*.c")
doomgeneric_files = Glob("thirdparty/doomgeneric/doomgeneric/*.c", exclude=[
    "thirdparty/doomgeneric/doomgeneric/doomgeneric_emscripten.c",
    "thirdparty/doomgeneric/doomgeneric/doomgeneric_sdl.c",
    "thirdparty/doomgeneric/doomgeneric/doomgeneric_soso.c",
    "thirdparty/doomgeneric/doomgeneric/doomgeneric_sosox.c",
    "thirdparty/doomgeneric/doomgeneric/doomgeneric_win.c",
    "thirdparty/doomgeneric/doomgeneric/doomgeneric_xlib.c",
    "thirdparty/doomgeneric/doomgeneric/i_sdlmusic.c",
    "thirdparty/doomgeneric/doomgeneric/i_sdlsound.c",
])
spawn_sources += doomgeneric_files

# Fluidsynth
env.Append(LIBS=['fluidsynth'])

root_addons = os.path.join(".", "addons", "godot-doom-node", env["platform"])
root_demo_addons = os.path.join("demo", root_addons)

library_name = ""
if env['platform'] == "macos":
    library_name = os.path.join(f"libgodot-doom-node.{env['platform']}.{env['target']}.framework", f"godot-doom-node.{env['platform']}.{env['target']}")
else:
    library_name = f"libgodot-doom-node{env['suffix']}{env['SHLIBSUFFIX']}"
library_path = os.path.join(root_addons, library_name)

library = env.SharedLibrary(
    library_path,
    source=sources,
)

program_path = os.path.join(root_addons, spawn_executable_name)
program = env.Program(
    program_path,
    source=spawn_sources
)

Default(library, program)

if env.get("compiledb", False):
    env.Tool("compilation_db")
    env.Alias("compiledb", env.CompilationDatabase(env.get("compiledb_file", None)))
