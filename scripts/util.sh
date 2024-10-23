export CLANG=clang-20
export OPT=opt

__emit() {
    $CLANG -O0 -emit-llvm -S -o - $1
}

_runpass() {
    opt <(__emit $1) -p $2 -o /tmp/a.bc
}
#$opt example1-1.ll -p print-lcg-dot -o /tmp/example1-1.o 2> >(dot -Tx11)