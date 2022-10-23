#include "kernel/types.h"

#include "kernel/stat.h"
#include "user/user.h"

#include "kernel/param.h"

int main(int argc, char *argv[]) {
  char *newargv[MAXARG];
  char line[512];
  char *p, *pre;
  int newargc = 0;
  int noblank;
  int pid;
  for (int i = 1; i < argc; i++) {
    newargv[newargc++] = argv[i];
  }

  line[0] = '\0';
  gets(line, 512);
  while (strlen(line) > 0) {
    // printf("getline:%s\n", line);

    for (noblank = 0, newargc = argc - 1, pre = p = line; *p; p++) {
      if (noblank) {
        if (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
          *p++ = '\0';
          noblank = 0;
          if (newargc == MAXARG)
            break;
          newargv[newargc++] = pre;
        }
      } else {
        if (*p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') {
          pre = p;
          noblank = 1;
        }
      }
    }
    if (newargc == MAXARG && pre != newargv[newargc - 1]) {
      printf("too many args\n");
      continue;
    }
    if (pre != newargv[newargc - 1]) {
      newargv[newargc++] = pre;
    }
    newargv[newargc] = 0;

    // for (int i = 0; i < newargc; i++) {
    //   printf("%s ", newargv[i]);
    // }
    // printf("\n");

    pid = fork();
    if (pid < 0) {
      fprintf(2, "fork error\n");
      exit(1);
    }
    if (pid == 0) {
      exec(newargv[0], newargv);
    } else {
      wait(0);
    }

    line[0] = '\0';
    gets(line, 512);
  }
  exit(0);
}
