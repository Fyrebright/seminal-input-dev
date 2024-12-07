#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

int odd(int);

// Print error message and exit
void error(const char *msg) {
    fprintf(stderr, "Error: %s\n", msg);
    exit(1);
}

int even(int n) {
    if (n < 0) {
        error("even: negative argument");
    }

    if(n == 0)
        return true;
    else
        return odd(n-1);
}

int odd(int n) {
    if (n < 0) {
        error("odd: negative argument");
    }

    if(n == 0)
        return false;
    else
        return even(n-1);
}

int main() {
    // Test multiple with scanf
    int used0,used1,used2,used3,used4,used5,used6,used7,unused0,unused1,unused2;
    printf("Eight integers pls...");
    scanf("%d, %d, %d, %d, %d, %d, %d, %d", &used0,&used1,&used2,&used3,&used4,&used5,&used6,&used7);

    printf("%d is odd?: %s", used0,odd(used0) ? "true" : "false");
    printf("%d is odd?: %s", used1,odd(used1) ? "true" : "false");
    printf("%d is odd?: %s", used2,odd(used2) ? "true" : "false");
    printf("%d is odd?: %s", used3,odd(used3) ? "true" : "false");

    printf("%d is even?: %s",used4, even(used4) ? "true" : "false");
    printf("%d is even?: %s",used5, even(used5) ? "true" : "false");
    printf("%d is even?: %s",used6, even(used6) ? "true" : "false");
    printf("%d is even?: %s",used7, even(used7) ? "true" : "false");

    // Excuse to use a returning input function instead
    printf("Enter a char: ");
    int amt = (int) getchar();
    int arr[amt];
    for (int i = 0; i < amt; i++) {
        // Input values with fgets instead
        printf("Enter an integer: ");
        char input[10];
        fgets(input, 10, stdin);
        arr[i] = atoi(input);
    }

    // only check part
    for (int i = 0; i < 10; i+=2) {
        printf("%d is odd?: %s", arr[i], odd(arr[i]) ? "true" : "false");
    }
}