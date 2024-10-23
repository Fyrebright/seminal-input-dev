int fun(int a) {
  if(a == 0) {
    return 0;
  } else if(a > 7) {
    return 1;
  } else {
    return 20;
  }
}

int main() {
  int (*fun_ptr)(int) = &fun;
  int c = (*fun_ptr)(10);

  for(int i = 0; i < 3; i++) {
    c = c + 1;
  }

  for(int i = 0; i < 3; i++) {
    c = c + 1;
  }
  return c;
}
