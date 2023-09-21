#!/usr/bin/env python

import subprocess

build_type = "Debug"

subprocess.run(["git", "submodule", "update", "--init", "--recursive"])
subprocess.run(["cmake", "-S", "submodules/llvm-project/llvm", "-B", "build/llvm-build", f"-DCMAKE_BUILD_TYPE={build_type}", "-DLLVM_ENABLE_PROJECTS=clang;lld"])
subprocess.run(["cmake", "--build", "build/llvm-build"])
