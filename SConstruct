#!/usr/bin/env python
import os
from glob import glob
from pathlib import Path

env = SConscript("godot-cpp/SConstruct")

# build with already installed static lib, also need to remove third_party_dir from CPPPath and maybe change LIBPATH
# env.Append(CPPPATH=["src/", "/usr/include"])
# env.Append(LIBPATH='/usr/lib')
# env.Append(LIBS=["osdGPU"])

# compile local
thirdparty_dir = "thirdparty/opensubdiv/"
thirdparty_sources = [
    "far/error.cpp",
    "far/topologyDescriptor.cpp",
    "far/topologyRefiner.cpp",
    "far/topologyRefinerFactory.cpp",
    "sdc/crease.cpp",
    "sdc/typeTraits.cpp",
    "vtr/fvarLevel.cpp",
    "vtr/fvarRefinement.cpp",
    "vtr/level.cpp",
    "vtr/quadRefinement.cpp",
    "vtr/refinement.cpp",
    "vtr/sparseSelector.cpp",
    "vtr/triRefinement.cpp",
]
thirdparty_sources = [thirdparty_dir +
                      file for file in thirdparty_sources]
env.Append(CPPPATH=["src/", thirdparty_dir])


sources = Glob("src/*.cpp")
sources.extend(thirdparty_sources)

# Find gdextension path even if the directory or extension is renamed.
(extension_path,) = glob("project/addons/*/*.gdextension")

# Find the addon path
addon_path = Path(extension_path).parent

# Find the extension name from the gdextension file (e.g. example).
extension_name = Path(extension_path).stem


# Create the library target
if env["platform"] == "osx":
    library = env.SharedLibrary(
        "{}/bin/lib{}.{}.{}.framework/{1}.{2}.{3}".format(
            addon_path,
            extension_name,
            env["platform"],
            env["target"],
        ),
        source=sources,
    )
else:
    library = env.SharedLibrary(
        "{}/bin/lib{}.{}.{}.{}{}".format(
            addon_path,
            extension_name,
            env["platform"],
            env["target"],
            env["arch_suffix"],
            env["SHLIBSUFFIX"],
        ),
        source=sources,
    )

Default(library)
