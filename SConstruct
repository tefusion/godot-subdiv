"""
For building basic release do scons target=template_debug within this folder.

For building tests with debug symbols do scons target=template_debug dev_build=yes -Q tests=1

For actual debugging, just start debugging the same way as debugging godot directly. The path for the executable would be something like
path/to/godot --editor --path ${workspaceFolder}/project 
"""
#!/usr/bin/env python
import os
from glob import glob
from pathlib import Path


env = SConscript("godot-cpp/SConstruct")

# build with already installed static lib, also need to remove third_party_dir from CPPPath and maybe change LIBPATH
# env.Append(CPPPATH=["src/", "/usr/include"])
# env.Append(LIBPATH='/usr/lib')
# env.Append(LIBS=["osdGPU"])
vars = Variables(None, ARGUMENTS)
vars.Add(BoolVariable("tests", "Build tests", False))
vars.Update(env)
run_tests = env.get('tests')

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

cpp_path = ["src/", thirdparty_dir]
if(run_tests):
    cpp_path.append(["thirdparty/doctest", "test/"])
env.Append(CPPPATH=cpp_path)

cpp_defines = []
if "CPPDEFINES" in env:
    cpp_defines = env["CPPDEFINES"]
cpp_defines.append("_USE_MATH_DEFINES")
if run_tests:
    cpp_defines.append("TESTS_ENABLED")
env.Append(CPPDEFINES=cpp_defines)

sources = Glob("src/*.cpp")
sources.extend(Glob("src/*/*.cpp"))
sources.extend(thirdparty_sources)

if (run_tests):
    sources.extend(Glob("test/*.cpp"))
    sources.extend(Glob("test/*/*.cpp"))


# Find gdextension path even if the directory or extension is renamed.
(extension_path,) = glob("project/addons/*/*.gdextension")

# Find the addon path
addon_path = "project/addons/godot_subdiv"

# Find the extension name from the gdextension file (e.g. example).
extension_name = "godot_subdiv"

# if you do dev builds it currently appends .dev. This isn't compatible with the gdextension file so removing here
build_suffix = env["suffix"].replace(".dev.", ".")

# Create the library target
if env["platform"] == "macos":
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
        "{}/bin/lib{}{}{}".format(
            addon_path,
            extension_name,
            build_suffix,
            env["SHLIBSUFFIX"]
        ),
        source=sources,
    )

Default(library)
