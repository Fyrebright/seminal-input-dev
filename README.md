# CSC412 Course Project

## Summary

This is an out-of-tree pass using the template from [sampsyo/llvm-pass-skeleton](https://github.com/sampsyo/llvm-pass-skeleton).

It is _currently_ built against `llvm` / `clang` version 17.0.6. This is because I have repeatedly had issues building later versions (even on Release), but I was able to get this to work.

I intend to either fix those issues or test with v19/20 before final submission (probably much sooner) so I hope this is acceptable for the time being.

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
make -C build/
```

### Running Passes

#### With `clang`

```sh
clang -g -O0 -fpass-plugin=branch-ptr-trace/PrintBranchPass.so examples/hello.c -o /tmp/a.out
```

#### With `opt`

I only include this since it is closer to the official documentation, and I have not included a sample LLVM source file.

```sh
opt file.ll -load=skeleton/SkeletonPass.so -p PrintBranches -o /tmp/a.bc
```
