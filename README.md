# CSC412 Course Project

## Summary

This is an out-of-tree pass using the template from [sampsyo/llvm-pass-skeleton](https://github.com/sampsyo/llvm-pass-skeleton).

**LLVM / Clang Version:** 19.1.2

### Sample Run

```c
todo input
```

```sh
todo cmd
```

```
todo out
```


## Building

This project builds a shared object library that can be loaded by `clang` or `opt`. It is not necessary to modify / add files in your LLVM source, however, you may need to set some variables as described in [Config](###Config)

### Configuration

TODO

### Building Passes

From repository root:

```sh
cmake -B build/
cmake --build build/
```

### Running Passes

#### With `clang`

```sh
clang -g -O0 -fpass-plugin=build/branch-ptr-trace/PrintBranchPass.so examples/hello.c -o /tmp/a.out
clang -g -O0 -fno-discard-value-names -fpass-plugin=build/branch-ptr-trace/PrintBranchPass.so examples/example2-1.c -o /tmp/a.out
```
