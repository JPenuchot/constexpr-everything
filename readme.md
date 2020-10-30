# Constexpr everything

I mean, really. Constexpr everything.

## How-to

Once you did the `git clone --recurse-submodules ...` thing, then:

```
./prepare.sh
mkdir llvm-project/build
cd llvm-project/build
cmake -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra" ../
make constexpr-everything
```

So you can `./constexpr-everything some-cpp-source.cpp`.

It will add `constexpr` everywhere it should (ie. everywhere it can).
