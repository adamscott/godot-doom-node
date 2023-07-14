#!/usr/bin/env python
import os
import sys
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
env.Append(CPPPATH=[os.path.abspath("src/"), os.path.abspath("thirdparty/doomgeneric")])
env.Append(CPPDEFINES=["FEATURE_SOUND_GODOT"])

sources = Glob("src/*.cpp")
doomgeneric_mus2mid_file = Glob("thirdparty/doomgeneric/doomgeneric/mus2mid.c")
doomgeneric_memio_file = Glob("thirdparty/doomgeneric/doomgeneric/memio.c")
sources += doomgeneric_mus2mid_file + doomgeneric_memio_file

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
env.Append(LIBPATH=[os.path.abspath(os.path.join("thirdparty", "fluidsynth", "build", "src"))])
env.Append(CPPPATH=[
    os.path.abspath(os.path.join("thirdparty", "fluidsynth", "build", "include")), 
    os.path.abspath(os.path.join("thirdparty", "fluidsynth", "include"))
])

if env["platform"] == "macos":
    library = env.SharedLibrary(
        "demo/bin/gddoom.{}.{}.framework/gddoom.{}.{}".format(
            env["platform"], env["target"], env["platform"], env["target"]
        ),
        source=sources,
    )
else:
    library = env.SharedLibrary(
        "demo/bin/gddoom{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )
program = env.Program(
    "demo/bin/gddoom-spawn{}".format(env["suffix"]),
    source=spawn_sources
)

Default(library, program)
# Default(program)

if env.get("compiledb", False):
    env.Tool("compilation_db")
    env.Alias("compiledb", env.CompilationDatabase(env.get("compiledb_file", None)))
