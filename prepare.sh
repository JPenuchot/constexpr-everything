#!/bin/sh

ln -rs constexpr-everything llvm-project/clang-tools-extra
echo "add_subdirectory(constexpr-everything)" >> llvm-project/clang-tools-extra/CMakeLists.txt
