#include <stdio.h>
#include <stdlib.h>

// Example 2.1
int main() {
  int id;
  int n;
  scanf("%d, %d", &id, &n);
  int s = 0;
  for(int i = 0; i < n; i++) {
    s += rand();
    // s += 1;
  }
  printf("id=%d; sum=%d; s=%d\n", id, n, s);
}
