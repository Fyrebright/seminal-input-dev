
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

void do_with_switch(int x, int y, char *op)
{   
    char code = '\0';
    if (strcmp(op, "add") == 0)
    {
        code = 'a';
    }
    else if (strcmp(op, "sub") == 0)
    {
        code = 's';
    }
    else
    {
        fprintf(stderr, "Invalid operation\n");
    }

    int result;
    switch(code)
    {
        case 'a':
            result = add(x, y);
            break;
        case 's':
            result = sub(x, y);
            break;
        default:
            fprintf(stderr, "Invalid operation\n");
            return;
    }
    printf("result is %d\n", result);
}

int main(int argc, char *argv[])
{
    //check if no arguments are passed
    if(argc <= 3)
    {
        fprintf(stderr,"No arguments passed\n");
        return 0;
    }

    int x = atoi(argv[2]);
    int y = atoi(argv[3]);
    print(x,y,add);
    print(x,y,sub);

    // call add if first argument is "add"
    int (*func)(int,int);
    if(strcmp(argv[1], "add") == 0)
    {
        func = add;
    }
    else
    {
        func = sub;
    }

    print(x, y, func);
    (*func)(x, y);

    do_with_switch(x, y, argv[1]);

    return 0;
}