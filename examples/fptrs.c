
#include <stdio.h>

int add(int i, int j)
{
   return (i + j);
}

int sub(int i, int j)
{
   return (i - j);
}

void print(int x, int y, int (*func)(int,int))
{
    printf("value is : %d\n", (*func)(x, y));
}

int main(int argc, char *argv[])
{
    int x=100, y=200;
    print(x,y,add);
    print(x,y,sub);

    // call add if first argument is 1
    int (*func)(int,int);
    if(argc == 1)
    {
        func = add;
    }
    else
    {
        func = sub;
    }

    print(x, y, func);
    (*func)(x, y);

    return 0;
}