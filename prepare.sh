#!/bin/sh

wget "https://github.com/llvm/llvm-project/releases/download/llvmorg-10.0.1/llvm-project-10.0.1.tar.xz"
tar xvf llvm-project-10.0.1.tar.xz
ln -rs constexpr-everything llvm-project-10.0.1/clang-tools-extra
echo "add_subdirectory(constexpr-everything)" >> llvm-project-10.0.1/clang-tools-extra/CMakeLists.txt
