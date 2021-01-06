/*

Copyright (c) 2020 Agenium Scale

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

/*

This little utility's purpose is to launch jobs via SSH with immediate return
on the client side. It works on Linux and Windows and is mainly used for
NSIMD (https://github.com/agenium-scale/nsimd) CI. It creates a group (a job
object on Windows) of processes so that they can be killed easily. When it
succeeds, it prints the identifier of the job and exist. This ID can then be
used to kill the job.

$ sshjob run ping -n 2000 127.0.0.1
5893756

$ sshjob kill 5893756

On Linux, the job is created using setsid(2) and the usual fork(2) + execv(2).
Moreover file descriptors 0, 1 and 2 are closed for the job so that is
detached from SSH. On Windows the situation is a little more complex. We use
a 2-stage process as we do not have found a simplier way:

- stage1: We create a process that does not inherit any handle to be detached
  from SSH with the CREATE_BREAKAWAY_FROM_JOB flag so that the new process
  is not within the SSH job object and hence is not killed by SSH.

- stage2: We create a new job object and assign the process from stage1 to
  the newly created job object. The stage1 process then creates a new job
  that finally executes what the user wants. The resulting process is created
  with two particularities:

  1. The created job object must have a handle that can be inherited. Indeed
     if it is not the case then the job object can never be referenced by
     any other process and therefore cannot be killed. A handle to the job
     must be alive.

  2. The final process is created and inherits all the handles of the stage1
     process including the handle to the job object.

We do not provide any way of compiling it as the main use of this C program
is to be copied through network to computers part of the CI and compiled there
quickly. This program has no dependencies and is as portable as possible.

Windows: cl /Ox /W3 /D_CRT_SECURE_NO_WARNINGS sshjob.c

Unix: cc -O2 -Wall sshjob.c -o sshjob

*/

#ifndef _MSC_VER
  #define _POSIX_C_SOURCE 2001L
  #include <sys/types.h>
  #include <signal.h>
  #include <errno.h>
  #include <unistd.h>
#else
  #define _WIN32_WINNT 0x0500
  #include <windows.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_LEN_RUN_CMD  2046  /* explanations below */

const char *argv0;

void note_on_buffers(FILE *out) {
  fprintf(out,
          "NOTE: On Windows, command line cannot exceed %d characters.\n"
          "      Counting the terminating NUL character gives %d. On\n"
          "      other systems, the limit is much higher but the purpose\n"
          "      of this program is to launch batches of commands, i.e.\n"
          "      scripts whose name with a few arguments does not exceed\n"
          "      the Windows limitation. Moreover using a small known\n"
          "      max size allows the code to work without mallocs which\n"
          "      simplifies the program which is written in pure C as we\n"
          "      need to use low-level POSIX and Win32 APIs directly.\n\n"
          , MAX_LEN_RUN_CMD + 1, MAX_LEN_RUN_CMD);
}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/*                       L I N U X     V E R S I O N                         */
/*                                                                           */
/* ------------------------------------------------------------------------- */

#ifndef _MSC_VER

int run_job(char *cmd) {
  pid_t pid = fork();
  if (pid == -1) {
    fprintf(stderr, "%s: error: cannot run command: %s\n", argv0,
            strerror(errno));
    return -1;
  } else if (pid == 0) {
    /* Within the child, no error checking as we close 0, 1 and 2 for being
       dettached from SSH. */
    close(0);
    close(1);
    close(2);
    setsid();
    char *argv[4];
    argv[0] = "sh";
    argv[1] = "-c";
    argv[2] = cmd;
    argv[3] = NULL;
    execvp(argv[0], argv);
  }
  printf("%ld\n", (long)pid);
  return 0;
}

int kill_job(const char *pid_) {
  pid_t pid = (pid_t)atol(pid_);
  if (kill(-pid, SIGTERM) != 0) {
    fprintf(stderr, "%s: error: cannot kill processes: %s\n", argv0,
            strerror(errno));
    return -1;
  } else {
    return 0;
  }
}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/*                     W I N D O W S    V E R S I O N                        */
/*                                                                           */
/* ------------------------------------------------------------------------- */

#else

#define MAX_LEN_DWORD 10

const char *strerror2(DWORD last_error) {
  static char buf[1024];
  DWORD len;
  len =
      FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                     0, last_error, 0, buf, sizeof(buf) - 1, NULL);
  buf[len] = 0;
  return buf;
}

int run_job_stage2(const char *job_name, const char *cmd) {
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  SECURITY_ATTRIBUTES sa;
  int ret = 0;
  char buf[MAX_LEN_RUN_CMD];
  HANDLE hJob;

  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa.lpSecurityDescriptor = NULL;
  sa.bInheritHandle = TRUE;
  hJob = CreateJobObjectA(&sa, job_name);
  if (hJob == 0) {
    fprintf(stderr, "%s: error: cannot create job of processes (stage2): %s\n",
            argv0, strerror2(GetLastError()));
    return -1;
  }

  HANDLE hMe = GetCurrentProcess();
  if (AssignProcessToJobObject(hJob, hMe) == 0) {
    fprintf(stderr, "%s: error: cannot assign process to job (stage2): %s\n",
            argv0, strerror2(GetLastError()));
    ret = -1;
    CloseHandle(hMe);
    goto free_job;
  }

  ZeroMemory(&si, sizeof(STARTUPINFO));
  si.cb = sizeof(STARTUPINFO);
  ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
  if (sizeof("cmd /C \"\"") + strlen(cmd) > MAX_LEN_RUN_CMD) {
    fprintf(stderr, "%s: error: command line too long (stage2): %s\n",
            argv0, strerror2(GetLastError()));
    ret = -1;
    goto free_job;
  }
  sprintf(buf, "cmd /C \"%s\"", cmd);
  if (CreateProcessA(NULL, buf, NULL, NULL, TRUE, CREATE_NO_WINDOW,
                     NULL, NULL, &si, &pi) == 0) {
    fprintf(stderr, "%s: error: cannot create process (stage2): %s\n", argv0,
            strerror2(GetLastError()));
    ret = -1;
  }

  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

free_job:
  CloseHandle(hMe);
  CloseHandle(hJob);

  return ret;
}

int run_job_stage1(const char *cmd) {
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  char buf[MAX_LEN_RUN_CMD];

  if (strlen(argv0) + strlen(cmd) + sizeof(" run2: ") + MAX_LEN_DWORD >
      MAX_LEN_RUN_CMD) {
    fprintf(stderr, "%s: error: cammand line too long for stage2 (stage1)\n",
            argv0);
    note_on_buffers(stderr);
    return -1;
  }
  DWORD id = GetTickCount();
  sprintf(buf, "%s run2:%u %s", argv0, id, cmd);

  ZeroMemory(&si, sizeof(STARTUPINFO));
  si.cb = sizeof(STARTUPINFO);
  ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
  if (CreateProcessA(NULL, buf, NULL, NULL, FALSE,
                     CREATE_NO_WINDOW | CREATE_NEW_CONSOLE |
                         CREATE_BREAKAWAY_FROM_JOB,
                     NULL, NULL, &si, &pi) == 0) {
    fprintf(stderr, "%s: error: cannot create process (stage1): %s\n", argv0,
            strerror2(GetLastError()));
    return -1;
  }

  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  printf("%u\n", id);
  return 0;
}

int kill_job(const char *job_name) {
  HANDLE hJob = OpenJobObjectA(JOB_OBJECT_TERMINATE, FALSE, job_name);
  if (hJob == 0) {
    fprintf(stderr, "%s: error: cannot open job: %s\n", argv0,
            strerror2(GetLastError()));
    return -1;
  }

  if (TerminateJobObject(hJob, 0) == 0) {
    fprintf(stderr, "%s: error: cannot terminate job: %s\n", argv0,
            strerror2(GetLastError()));
    CloseHandle(hJob);
    return -1;
  }
  CloseHandle(hJob);
  return 0;
}

#endif

/* ------------------------------------------------------------------------- */
/*                                                                           */
/*                        E N T R Y    P O I N T                             */
/*                                                                           */
/* ------------------------------------------------------------------------- */

int main(int argc, char **argv) {
  argv0 = argv[0];
  char buf[MAX_LEN_RUN_CMD];

  if (argc == 1 || (argc == 2 && !strcmp(argv[1], "--help"))) {
    printf("%s: usage: %s (run command|kill id)\n", argv[0], argv[0]);
    return 0;
  }

  if (strcmp(argv[1], "run") && strcmp(argv[1], "kill") &&
      memcmp(argv[1], "run2:", 5)) {
    fprintf(stderr, "%s: error: unknown action: %s\n", argv[0], argv[1]);
    return -1;
  }

  if (!strcmp(argv[1], "run") || !memcmp(argv[1], "run2:", 5)) {
    size_t sz;
    int i;

    if (argc < 3) {
      fprintf(stderr, "%s: error: expected command to run\n", argv[0]);
      return -1;
    }
    sz = 0;
    for (i = 2; i < argc; i++) {
      char *src;
      for (src = argv[i]; src[0] && sz < MAX_LEN_RUN_CMD; src++, sz++) {
        buf[sz] = src[0];
      }
      if (i < argc - 1 && sz < MAX_LEN_RUN_CMD) {
        buf[sz] = ' ';
        sz++;
      }
    }
    if (sz < MAX_LEN_RUN_CMD) {
      buf[sz] = 0;
      sz++;
    }
    if (sz >= MAX_LEN_RUN_CMD && buf[MAX_LEN_RUN_CMD - 1] != 0) {
      fprintf(stderr, "%s: error: command exceeds %d characters\n", argv[0],
              MAX_LEN_RUN_CMD);
      note_on_buffers(stderr);
      return -1;
    }
#ifdef _MSC_VER
    if (!strcmp(argv[1], "run")) {
      return run_job_stage1(buf);
    } else {
      return run_job_stage2(argv[1] + 5, buf);
    }
#else
    return run_job(buf);
#endif
  } else {
    if (argc != 3) {
      fprintf(stderr, "%s: error: expected group process id\n", argv[0]);
      return -1;
    }
    return kill_job(argv[2]);
  }
}
