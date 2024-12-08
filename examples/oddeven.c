#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// Print error message and exit
void error(const char *msg) {
  fprintf(stderr, "Error: %s\n", msg);
  exit(1);
}

char even(int);

int odd(int n) {
  if(n < 0) {
    error("odd: negative argument");
  }

  if(n == 0)
    return 'F';
  else
    return even(n - 1);
}

char even(int n) {
  if(n < 0) {
    error("even: negative argument");
  }

  if(n == 0)
    return 'T';
  else
    return odd(n - 1);
}

int main() {
  // Test multiple with scanf
  int used0, used1, used2, used3, used4, used5, used6, used7, unused0, unused1,
      unused2;
  printf("Eight integers pls...");
  scanf("%d, %d, %d, %d, %d, %d, %d, %d",
        &used0,
        &used1,
        &used2,
        &used3,
        &used4,
        &used5,
        &used6,
        &used7);

  printf("%d is odd?: %c", used0, odd(used0));
  printf("%d is odd?: %c", used1, odd(used1));
  printf("%d is odd?: %c", used2, odd(used2));
  printf("%d is odd?: %c", used3, odd(used3));

  printf("%d is even?: %c", used4, even(used4));
  printf("%d is even?: %c", used5, even(used5));
  printf("%d is even?: %c", used6, even(used6));
  printf("%d is even?: %c", used7, even(used7));

  // Excuse to use a returning input function instead
  printf("Enter a char: ");
  int amt = (int)getchar();
  int arr[amt];
  for(int i = 0; i < amt; i++) {
    // Input values with fgets instead
    printf("Enter an integer: ");
    char input[10];
    fgets(input, 10, stdin);
    arr[i] = atoi(input);
  }

  // only check part
  for(int i = 0; i < 10; i += 2) {
    printf("%d is odd?: %c", arr[i], odd(arr[i]));
  }
}