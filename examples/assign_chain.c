#include <stdlib.h>
#include <stdio.h>

int calc() {
    int a = (getchar() - '0');
    // int a = getchar();

    int b = a;

    int c = b;

    int d = c;

    return d;
}

int main() {
    int d = calc();

    if(d==2)
        return 0;
    else
     return 1;
}