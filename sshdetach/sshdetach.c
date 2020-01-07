/*

Copyright (c) 2019 Agenium Scale

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

/* ------------------------------------------------------------------------- */

#ifdef _MSC_VER
#include <windows.h>
#else
#define _POSIX_C_SOURCE 200112L
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

#include <stdio.h>

const char *argv0;

/* ------------------------------------------------------------------------- */
/* WINDOWS CASE HERE */

#ifdef _MSC_VER

const char *strerror2(DWORD last_error) {
  static char buf[1024];
  DWORD len;
  len =
      FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                     0, last_error, 0, buf, sizeof(buf) - 1, NULL);
  buf[len] = 0;
  return buf;
}

int spawn(const char *command) {
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  int ret;
  char *cmdline;
  size_t command_len;

  /* Fill the STARTUPINFO struct */
  ZeroMemory(&si, sizeof(STARTUPINFO));
  si.cb = sizeof(STARTUPINFO);

  /* Prepare command line */
  command_len = strlen(command);
  cmdline = malloc(command_len + 10);
  if (cmdline == NULL) {
    fprintf(stderr,
            "%s: error: cannot create command line: not enough memory\n",
            argv0);
    return -1;
  }
  memcpy(cmdline, "cmd /C \"", 8);
  memcpy(cmdline + 8, command, command_len);
  cmdline[8 + command_len] = '"';
  cmdline[8 + command_len + 1] = 0;

  /* Launch the process */
  memset(&pi, 0, sizeof(pi));
  if (CreateProcessA(NULL, cmdline, NULL, NULL, FALSE,
                     CREATE_NO_WINDOW | CREATE_NEW_CONSOLE |
                         CREATE_BREAKAWAY_FROM_JOB,
                     NULL, NULL, &si, &pi) == 0) {
    fprintf(stderr, "%s: error: cannot create process: %s\n", argv0,
            strerror2(GetLastError()));
    ret = -1;
  } else {
    ret = 0;
  }

  /* Close all handles and free resources, we don't care about our child */
  if (ret == 0) {
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
  }
  free(cmdline);

  return ret;
}

#else

/* ------------------------------------------------------------------------- */
/* LINUX CASE HERE */

int spawn(const char *command) {
  pid_t child = fork();
  if (child == 0) {

    /* Within the child here */
    pid_t grandchild = fork();
    if (grandchild == 0) {
      const char *cmdline[4];

      /* Within the grand child */
      close(0);
      close(1);
      close(2);

      /* Construct command line */
      cmdline[0] = "/bin/sh";
      cmdline[1] = "-c";
      cmdline[2] = command;
      cmdline[3] = NULL;

      /* Execvp the new command */
      if (execv(cmdline[0], (char *const *)cmdline) == -1) {
        fprintf(stderr, "%s: error: cannot execv: %s\n", argv0,
                strerror(errno));
        exit(EXIT_FAILURE);
      }

    } else if (grandchild == -1) {

      /* Error when forking */
      fprintf(stderr, "%s: error: cannot fork child: %s\n", argv0,
              strerror(errno));
      exit(EXIT_FAILURE);

    } else {

      /* Child has to die */
      exit(EXIT_FAILURE);

    }

  } else if (child == -1) {

    /* Error when forking */
    fprintf(stderr, "%s: error: cannot fork child: %s\n", argv0,
            strerror(errno));
    return -1;

  } else {

    /* In the parent we wait for our child */
    int status;
    int code = waitpid(child, &status, 0);
    if (code == -1) {
      fprintf(stderr, "%s: error: cannot wait for my child: %s\n",
              argv0, strerror(errno));
    }

  }

  return 0;
}

#endif

/* ------------------------------------------------------------------------- */

int main(int argc, char **argv) {
  if (argc != 2 && argc && 3) {
    fprintf(stderr, "%s: error: wrong number of arguments\n"
                    "%s: usage: %s PROGRAM [OUTPUT_FILE]\n",
            argv[0], argv[0], argv[0]);
    exit(EXIT_FAILURE);
  }
  argv0 = argv[0];
  if (spawn(argv[1]) == -1) {
    fprintf(stderr, "%s: error: unable to spawn %s\n", argv[0], argv[1]);
    return -1;
  }
  fprintf(stderr, "%s: spawned %s\n", argv[0], argv[1]);
  return 0;
}
