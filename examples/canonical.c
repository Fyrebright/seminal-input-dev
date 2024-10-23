void test(int n) {
  int i = 0;
  if(n > 0) {
    do {
      // Loop body
      int a = 1;
      a = 1 - a;
      i += 1;
    } while(i < n);
  }
}

int main() {
  test(5);
}
