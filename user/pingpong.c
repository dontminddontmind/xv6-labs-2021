#include "kernel/types.h"

#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {

  int f2c[2], c2f[2];
  if (pipe(f2c) == -1) {
    fprintf(2, "pipe error\n");
    exit(0);
  }
  if (pipe(c2f) == -1) {
    fprintf(2, "pipe error\n");
    exit(0);
  }
  int pid = fork();
  if (pid == 0) {  // child
    close(f2c[1]); // close father write

    char c;
    read(f2c[0], &c, 1);
    fprintf(1, "%d: received ping\n", getpid());
    write(c2f[1], &c, 1);

  } else {
    close(c2f[1]); // close child write
    write(f2c[1], "a", 1);
    char c;
    read(c2f[0], &c, 1);
    fprintf(1, "%d: received pong\n", getpid());
  }

  exit(0);
}
