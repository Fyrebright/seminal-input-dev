# Seminal Input Detection with LLVM

## Summary

_Originally a project for a Compiler Construction Course:_ Static analysis tool implemented as an LLVM analysis pass in C++ for identifying _seminal input values_ (values derived from user input that affect the amount time it takes to execute a program) in C source files.

Seminal Behaviors described in:
>Yunlian Jiang, Eddy Z Zhang, Kai Tian, Feng Mao, Malcom Gethers, Xipeng Shen, and Yaoqing Gao. 2010. Exploiting statistical correlations for proactive prediction of program behaviors. Association for Computing Machinery, Toronto, Ontario, Canada, 248–256. DOI:https://doi.org/10.1145/1772954.1772989

This is an out-of-tree pass organized as in [banach-space/llvm-tutor](https://github.com/banach-space/llvm-tutor).

The two large examples, `examples/tic_tac_toe.c` and `examples/naval_battle.c`, are from [TheAlgorithms/C](https://github.com/TheAlgorithms/C).`examples/fopen_example.c` is adapted from [CPPReference](https://en.cppreference.com/w/c/io/fopen). The rest are my own or modified from the project description.


**LLVM / Clang Version:** 19.1.2


## Building

This project builds a shared object library that can be loaded by  `opt`. It is not necessary to modify / add files in your LLVM source, however, you may need to set some variables as described in [Config](###Config)

### Config

The `.envrc` is what I used to configure environment variables for both building LLVM and while developing the project. The variables are commented and I think most can be ignored but sometimes `LLVM_DIR` needs to be set for `LLVM REQUIRED CONFIG` to work. 

Obviously, also need to make sure headers are visible. Might need to change `LLVM_HOME` since I made it `~/.local` instead of `/usr/local`

### Building Passes

From repository root:

```sh
cmake -B build/
cmake --build build/
```

### Running Passes

Examples are included in `examples/`

#### The easy way

I have a script ready that will build and run the passes on a specified C source file.

To run the seminal input detection:

```sh
./inline_run.sh C_SRC.c
```

To see output for the other passes or the def-use graph that I used for debugging you can optionally specify a MODE argument

```sh
./inline_run.sh C_SRC.c MODE
```

**Modes:** 
* 0: `print<input-vals>` (old)
* 1: `print<find-key-pts>`
* 2: `print<find-io-val>`
* 3: `print<seminal-input>`
* 4: `graph<seminal-input>`(prints all functions together, need to separate or remove them)


#### With `opt`

The passes will be built as shared libraries to be loaded with `opt` e.g.

```sh
opt \
    -load-pass-plugin=$PLUGIN_PATH \
    -passes=$PASSES \
    -disable-output \
    ir-file.ll
```

The plugins and the corresponding passes are:

* `build/lib/libFindIOVals.so`: ` print<find-io-val>`
* `build/lib/libFindIOVals.so`: `print<find-key-pts>`
* `build/lib/libFindIOVals.so`: `print<seminal-input>, graph<seminal-input>`
