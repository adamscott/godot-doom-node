#!/usr/bin/env python
import shutil
import os
import sys

def main():
    if len(sys.argv) != 3:
        print("Must pass a target and a spawn executable name as command line argument")
        exit(1)
    
    library = sys.argv[1]
    executable = sys.argv[2]

    root_addons = os.path.join("addons", "godot-doom-node")
    root_demo_addons = os.path.join("demo", root_addons)

    platform = ""

    if sys.platform == "darwin":
        platform = "macos"
    elif sys.platform == "linux":
        platform = "linux"
    elif sys.platform == "win32":
        platform = "windows"

    shutil.copyfile(
        os.path.join(root_addons, platform, library),
        os.path.join(root_demo_addons, platform, library),
    )
    shutil.copyfile(
        os.path.join(root_addons, platform, executable),
        os.path.join(root_demo_addons, platform, executable),
    )

    shutil.copyfile(
        os.path.join(root_addons, "godot-doom.gdextension"),
        os.path.join(root_demo_addons, "godot-doom.gdextension")
    )

if __name__ == "__main__":
    main()
