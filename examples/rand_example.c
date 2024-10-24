#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned long next = 1;
int myrand(void) /* RAND_MAX assumed to be 32767. */
{
  next = next * 1103515245 + 12345;
  return ((unsigned)(next / 65536) % 32768);
}

void mysrand(unsigned seed) {
  next = seed;
}

int main() {
  long count, i;
  char *keystr;
  int elementlen, len;
  char c;
  /* Initial random number generator. */
  srand(1);

  /* Create keys using only lowercase characters */
  len = 0;
  for(i = 0; i < count; i++) {
    while(len < elementlen) {
      c = (char)(rand() % 128);
      if(islower(c))
        keystr[len++] = c;
    }

    keystr[len] = '\0';
    printf("%s Element%0*ld\n", keystr, elementlen, i);
    len = 0;
  }
}