#!/usr/bin/env python
import os

from SCons.Errors import UserError

import methods


def normalize_path(val):
    return val if os.path.isabs(val) else os.path.join(env.Dir("#").abspath, val)


def validate_compiledb_file(key, val, env):
    if not os.path.isdir(os.path.dirname(normalize_path(val))):
        raise UserError(
            "Directory ('%s') does not exist: %s" % (key, os.path.dirname(val))
        )


def get_compiledb_file(env):
    return normalize_path(env.get("compiledb_file", "compile_commands.json"))


env = Environment()
customs = [os.path.abspath("custom.py")]
opts = Variables(customs, ARGUMENTS)
opts.Add("fluidsynth_lib_path", "Path to fluidsynth lib path", "")
opts.Add("fluidsynth_include_path", "Path to fluidsynth lib path", "")
opts.Update(env)

clonedEnv = env.Clone()
clonedEnv["compiledb"] = False
env = SConscript("godot-cpp/SConstruct", {"env": clonedEnv, "customs": customs})

# For reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags

# tweak this if you want to use different folders, or more folders, to store your source code in.

env.Append(CPPPATH=[os.path.abspath("src/"), os.path.abspath("thirdparty/doomgeneric")])
env.Append(CPPDEFINES=["FEATURE_SOUND", "FEATURE_SOUND_GODOT"])

sources = Glob("src/*.cpp")
sources += Glob("src/*.c")
doomgeneric_files = Glob(
    "thirdparty/doomgeneric/doomgeneric/*.c",
    exclude=[
        "thirdparty/doomgeneric/doomgeneric/doomgeneric_allegro.c",
        "thirdparty/doomgeneric/doomgeneric/doomgeneric_emscripten.c",
        "thirdparty/doomgeneric/doomgeneric/doomgeneric_sdl.c",
        "thirdparty/doomgeneric/doomgeneric/doomgeneric_soso.c",
        "thirdparty/doomgeneric/doomgeneric/doomgeneric_sosox.c",
        "thirdparty/doomgeneric/doomgeneric/doomgeneric_win.c",
        "thirdparty/doomgeneric/doomgeneric/doomgeneric_xlib.c",
        "thirdparty/doomgeneric/doomgeneric/i_allegromusic.c",
        "thirdparty/doomgeneric/doomgeneric/i_allegrosound.c",
        "thirdparty/doomgeneric/doomgeneric/i_sdlmusic.c",
        "thirdparty/doomgeneric/doomgeneric/i_sdlsound.c",
    ],
)

doomgeneric_env = env.Clone()
if methods.using_gcc(doomgeneric_env):
    doomgeneric_env.Append(
        CCFLAGS=["-Wno-discarded-qualifiers"], LINKFLAGS=["-Wno-discarded-qualifiers"]
    )
elif methods.using_clang(doomgeneric_env):
    doomgeneric_env.Append(
        CCFLAGS=["-Wno-ignored-qualifiers"], LINKFLAGS=["-Wno-ignored-qualifiers"]
    )
doomgeneric_library = doomgeneric_env.StaticLibrary(
    "doomgeneric", source=doomgeneric_files
)
env.Append(LIBS=[doomgeneric_library])

env.Append(
    LIBS=["fluidsynth"],
    LIBPATH=[env["fluidsynth_lib_path"]],
    CPPPATH=[env["fluidsynth_include_path"]],
)

# Fluidsynth
if env["platform"] == "windows":
    env.Append(LIBS=["kernel32", "user32"])

root_addons = os.path.join(".", "addons", "godot-doom-node", env["platform"])
root_demo_addons = os.path.join("demo", root_addons)

library_name = ""
if env["platform"] == "macos":
    library_name = os.path.join(
        f"libgodot-doom-node.{env['platform']}.{env['target']}.framework",
        f"godot-doom-node.{env['platform']}.{env['target']}",
    )
else:
    library_name = f"libgodot-doom-node{env['suffix']}{env['SHLIBSUFFIX']}"
library_path = os.path.join(root_addons, library_name)

library = env.SharedLibrary(library_path, source=sources, library=[doomgeneric_library])

Default(library)
