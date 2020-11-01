# Constexpr everything

I mean, really. Constexpr everything.

## What it does

`constexpr-everything` makes all function declarations `constexpr` as long as
they satisfy `constexpr` requirements, and outputs clang's diagnosis when it
can't be done properly (ie. when all paths lead to a non-constexpr result).

For example:

```cpp
#include <type_traits>

template <typename T> int temp() { return 43; }

int foo() {
  if (!std::is_constant_evaluated())
    std::abort();
  return 2;
}

int foo_bis() {
  std::abort();
  return 2;
}

int bar() {
  struct some_struct_t {
    int yum() { return 7; }
  };

  return 4;
}
```

becomes:

```cpp
#include <type_traits>

template <typename T> constexpr int temp() { return 43; }

constexpr int foo() {
  if (!std::is_constant_evaluated())
    std::abort();
  return 2;
}

int foo_bis() {
  std::abort();
  return 2;
}

constexpr int bar() {
  struct some_struct_t {
    constexpr int yum() { return 7; }
  };

  return 4;
}
```

## How-to

Once you did the `git clone --recurse-submodules ...` thing, then:

```
./prepare.sh
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra" ../llvm-project/llvm
make constexpr-everything
```

So you can `constexpr-everything -p build_path/ source_a.cpp source_b.cpp`.

It will add `constexpr` everywhere it should (ie. everywhere it can without
ever breaking your code).

**Important**: The tool is intended to be run on **source** files, not header
files, especially if you have function declarations and definitions separated
across multiple files. It will transform your included files recursively
(except system headers), so you might want think twice before running this
tool.

See `--help`:

```
USAGE: constexpr-everything [options] <source0> [... <sourceN>]

OPTIONS:

Constexpr-everything Options:

  --diagnose-on-failure       - Emit a diagnosis when a function doesn't meet constexpr requirements.
  --extra-arg=<string>        - Additional argument to append to the compiler command line
  --extra-arg-before=<string> - Additional argument to prepend to the compiler command line
  -p=<string>                 - Build path

Generic Options:

  --help                      - Display available options (--help-hidden for more)
  --help-list                 - Display list of available options (--help-list-hidden for more)
  --version                   - Display the version of this program

-p <build-path> is used to read a compile command database.

  For example, it can be a CMake build directory in which a file named
  compile_commands.json exists (use -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
  CMake option to get this output). When no build path is specified,
  a search for compile_commands.json will be attempted through all
  parent paths of the first input file . See:
  https://clang.llvm.org/docs/HowToSetupToolingForLLVM.html for an
  example of setting up Clang Tooling on a source tree.

<source0> ... specify the paths of source files. These paths are
  looked up in the compile command database. If the path of a file is
  absolute, it needs to point into CMake's source tree. If the path is
  relative, the current working directory needs to be in the CMake
  source tree and the file must be in a subdirectory of the current
  working directory. "./" prefixes in the relative files will be
  automatically removed, but the rest of a relative path must be a
  suffix of a path in the compile command database.
```

### Notes

- `compile_commands.json` shoud be present in the build directory specified
with `-p` and can be generated with `cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON`.

- Making your functions constexpr will make them inline as well.
