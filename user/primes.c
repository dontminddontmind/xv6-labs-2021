#include "kernel/types.h"

#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  int p[3][2];
  int pi = 0;
  int pid = 0;
  for (int i = 0; i < 2; i++) {
    if (pipe(p[i]) == -1) {
      fprintf(2, "pipe %d error\n", i);
      exit(0);
    }
  }
  for (int i = 2; i <= 35; i++) {
    write(p[pi][1], &i, sizeof(int));
  }
  close(p[pi][1]);

  int prime = 0, num;
  int mkfork = 0;
  do {
    mkfork = 0;
    while (read(p[pi][0], &num, sizeof(int))) {
      if (prime == 0) {
        prime = num;
        fprintf(1, "prime %d\n", prime);
      } else if (num % prime) {
        if (pid == 0) {
          mkfork = 1;
        }
        write(p[(pi + 1) % 3][1], &num, sizeof(int));
      }
    }
    close(p[(pi + 1) % 3][1]);
    close(p[pi][0]);
    if (mkfork) {
      pid = fork();
      if (pid == 0) {
        pi = (pi + 1) % 3;
        if (pipe(p[(pi + 1) % 3]) == -1) {
          fprintf(2, "pipe %d error\n", (pi + 1) % 3);
          exit(0);
        }
        prime = 0;
      }
    }
  } while (pid == 0 && mkfork == 1);

  wait(0);
  exit(0);
}
