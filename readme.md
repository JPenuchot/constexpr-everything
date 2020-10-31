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
mkdir llvm-project/build
cd llvm-project/build
cmake -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra" ../
make constexpr-everything
```

So you can `./constexpr-everything -p build_path/ source_a.cpp source_b.cpp`.

It will add `constexpr` everywhere it should (ie. everywhere it can without
ever breaking your code).

### Notes

- `compile_commands.json` shoud be present in the build directory specified
with `-p` and can be generated with `cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON`.

- Making your functions constexpr will make them inline as well.
