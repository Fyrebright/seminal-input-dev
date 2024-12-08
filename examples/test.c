#include <cstdlib>
#include <stdio.h>
#include <unistd.h>

int main() {


  // --- After 2
  // sscanf(const char *__restrict s, const char *__restrict format, ...);
  // fscanf(FILE *__restrict stream, const char *__restrict format, ...);

  // --- After 1
  // scanf(const char *__restrict format, ...);
  
  // --- first arg
  // gets(char *str);
  // fgets(char *__restrict s, int n, FILE *__restrict stream);
  // fread(void *__restrict ptr, size_t size, size_t n, FILE *__restrict stream); 
  
  // --- third arg
  // int getopt(int argc, char *const argv[], const char *optstring);


  // --- Return val
  // FILE *fd = fopen("foo.txt", "r");
  // fdopen(int fd, const char *modes);
  // freopen(const char *__restrict filename, const char *__restrict modes, FILE *__restrict stream);
  // popen(const char *command, const char *modes);

  // fgetc(FILE *stream);
  // getc(FILE *stream);
  // getc_unlocked(FILE *stream);
  // getchar();
  // getchar_unlocked();
  // getw(FILE *stream);
  // getenv(const char *name);
  
  // --------------------------- ??
  // ungetc(int c, FILE *stream);
  // fgetpos(FILE *__restrict stream, fpos_t *__restrict pos);  
}