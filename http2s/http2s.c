/*

Copyright (c) 2019 Agenium Scale

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

/*****************************************************************************/

#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/select.h>

/*****************************************************************************/

#define MAX_LEN  1024
#define MAX_TRY  1024
#define ADDR_LEN 4096
#define IP_LEN   32

FILE *flog;
int port;
char *ip;
int urlc; char **urlv;
int daemonize;
char hostname[MAX_LEN];
uid_t uid;
gid_t gid;
char *log_filename;
int ss;
int verbose;
int iptables_block;
int listi0, listi1; char **listv;
char *focus_on_ip;
unsigned long failure_interval, failure_max;

struct thread_arg {
  int ns;
  int logging;
  char ip_str[IP_LEN];
};

struct failure {
  unsigned long count;
  unsigned long t0;
};

pthread_mutex_t mutex_fails;
struct failure fails[1 << 24];

pthread_mutex_t mutex_vlogger;

/*****************************************************************************/

void vlogger(const char *format, va_list ap) {
  time_t t_now;
  char *asc_now, *lf;
  int error;

  error = pthread_mutex_lock(&mutex_vlogger);

  t_now = time(NULL);
  asc_now = asctime(localtime(&t_now));
  lf = strchr(asc_now,'\n');
  if (lf) lf[0] = 0;
  fprintf( flog
         , "%s %s http2s[%lu]: "
         , asc_now
         , hostname
         , (long)getpid()
         );
  vfprintf(flog, format, ap);
  fputc('\n', flog);
  fflush(flog);

  if (error == 0) pthread_mutex_unlock(&mutex_vlogger);
}

/*****************************************************************************/

void logger(const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  vlogger(format, ap);
  va_end(ap);
}

/*****************************************************************************/

void exit2(int exit_code) {
  if (flog != stderr) fclose(flog);
  if (ss != -1) close(ss);
  exit(exit_code);
}

/*****************************************************************************/

void die(const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  vlogger(format, ap);
  va_end(ap);
  logger("exiting now");
  exit2(EXIT_FAILURE);
}

/*****************************************************************************/

void init(int argc, char **argv) {
  int i;

  flog = stderr;
  port = 80;
  ip = "0.0.0.0";
  urlc = 0;
  urlv = NULL;
  daemonize = 0;
  uid = getuid();
  if (gethostname(hostname, MAX_LEN)) strcpy(hostname,"(none)");
  log_filename = "/var/log/http2s.log";
  ss = -1;
  verbose = 0;
  listi0 = -1;
  listi1 = -1;
  listv = argv;
  focus_on_ip = NULL;
  iptables_block = 0;
  failure_interval = 1;
  failure_max = 5;

  for (i = 0; i < (1 << 24); i++) {
    fails[i].t0 = 0;
    fails[i].count = 0;
  }

  for (i = 0; i < argc; i++) {
    if (!strcmp(argv[i], "-d")) {
      daemonize = 1;
      continue;
    }
    if (!strcmp(argv[i], "-v")) {
      verbose = 1;
      continue;
    }
    if (!strcmp(argv[i], "-b")) {
      iptables_block = 1;
      continue;
    }
    if (!strcmp(argv[i], "-fi")) {
      i++;
      if (i >= argc) die("no interval given after -fi");
      failure_interval = (unsigned long)atol(argv[i]);
      continue;
    }
    if (!strcmp(argv[i], "-fm")) {
      i++;
      if (i >= argc) die("no maximum given after -fm");
      failure_max = (unsigned long)atol(argv[i]);
      continue;
    }
    if (!strcmp(argv[i], "-ip")) {
      i++;
      if (i >= argc) die("no IP address given after -ip");
      if (inet_addr(argv[i]) == INADDR_NONE) {
        die("%s: invalid IP address", argv[i]);
      }
      ip = argv[i];
      continue;
    }
    if (!strcmp(argv[i], "-focus")) {
      i++;
      if (i >= argc) die("no IP address given after -focus");
      focus_on_ip = argv[i];
      continue;
    }
    if (!strcmp(argv[i], "-log")) {
      i++;
      if (i >= argc) die("no log file given after -log");
      log_filename = argv[i];
      continue;
    }
    if (!strcmp(argv[i], "-p")) {
      i++;
      if (i >= argc) die("no port given after -p");
      port = atoi(argv[i]);
      if (port <= 0 || port > 65535) {
        die("%d: wrong port number", port);
      }
      continue;
    }
    if (!strcmp(argv[i], "-u")) {
      struct passwd *p;
      i++;
      if (i >= argc) die("no username given after -u");
      p = getpwnam(argv[i]);
      if (p == NULL) die("%s: wrong username", argv[i]);
      uid = p->pw_uid;
      continue;
    }
    if (!strcmp(argv[i],"-g")) {
      struct group *p;
      i++;
      if (i >= argc) die("no groupname given after -g");
      p = getgrnam(argv[i]);
      if (p == NULL) die("%s: wrong groupname", argv[i]);
      gid = p->gr_gid;
      continue;
    }
    if (!strcmp(argv[i], "--")) {
      i++;
      listi0 = i;
      listi1 = argc;
      break;
    }
  }

  if (listi1 - listi0 <= 0) die("no URLs given");
}

/*****************************************************************************/

void dropdown_privileges(void) {
  errno = 0;
  if (setuid(uid) == -1) {
    die("cannot change user: %s", strerror(errno));
  }

  errno = 0;
  if (setgid(gid) == -1) {
    die("cannot change group: %s", strerror(errno));
  }
}

/*****************************************************************************/

void do_daemonize() {
  pid_t pid1, pid2;
  FILE *local;

  /* we open here the log file as it the only mean to have feedback */
  errno = 0;
  local = fopen(log_filename, "a");
  if (local == NULL) {
    die( "%s: cannot open log file: %s"
       , log_filename
       , strerror(errno)
       );
  }
  flog = local;

  /* now we daemonize */
  errno = 0;
  pid1 = fork();
  if (pid1 == -1) die("cannot fork: %s", strerror(errno));
  if (pid1 > 0) {
    /* in the parent */
    waitpid(pid1, NULL, 0);
    fclose(local);
    exit2(EXIT_SUCCESS);
  }
  else {
    /* in the child */
    errno = 0;
    if (setsid() == -1) die("cannot setsid: %s", strerror(errno));
    flog = local; /* from here: no controlling terminal */
    errno = 0;
    pid2 = fork();
    if (pid2 == -1) die("cannot fork: %s", strerror(errno));
    if (pid2 == 0) return; /* in the grandchild => return */
    exit2(EXIT_SUCCESS);
  }
}

/*****************************************************************************/

void new_fail(const char *ip_str) {
  unsigned int ip[4] = { 0, 0, 0, 0 };
  int i, error;
  const char *s;
  struct timespec time_now;
  unsigned long now;

  error = pthread_mutex_lock(&mutex_fails);

  for (s = ip_str, i = 0; s[0]; s++, i++) {
    for (; s[0] && s[0] != '.'; s++) {
      ip[i] = 10 * ip[i] + (s[0] - '0');
    }
  }
  i = (ip[0] << 16) | ((ip[1] ^ ip[3]) << 8) | ip[2];

  clock_gettime(CLOCK_MONOTONIC, &time_now);
  now = (unsigned long)time_now.tv_sec;

  if ((now - fails[i].t0) <= failure_interval) {
    fails[i].count++;
    if (fails[i].count >= failure_max) {
      if (iptables_block) {
        char buf[256];
        sprintf(buf, "iptables -I INPUT -s %s -j DROP", ip_str);
        system(buf);
        logger("%s: blacklisted via iptables", ip_str);
      }
      fails[i].count = 0;
    }
  }
  else {
    fails[i].count = 1;
  }
  fails[i].t0 = now;

  if (error == 0) pthread_mutex_unlock(&mutex_fails);
}

/*****************************************************************************/

int fgetc2(int s) {
  fd_set rdfs;
  struct timeval tv;
  char b;

  FD_ZERO(&rdfs);
  FD_SET(s, &rdfs);
  tv.tv_sec = 2;
  tv.tv_usec = 0;

  if (select(s + 1, &rdfs, NULL, NULL, &tv) <= 0) return EOF;
  if (read(s, &b, 1) != -1) return b;
  return EOF;
}

/*****************************************************************************/

void *handle_connection(void *arg) {
  int listi, try, ns, logging;
  char path[ADDR_LEN];
  struct thread_arg *targ;
  const char *ip_str;

  if (pthread_detach(pthread_self())) {
    free(arg);
    return NULL;
  }

  targ = (struct thread_arg*)arg;
  ns = targ->ns;
  logging = targ->logging;
  ip_str = targ->ip_str;

  if (fcntl(ns, F_SETFL, O_NONBLOCK) == -1) {
    close(ns);
    free(arg);
    return NULL;
  }

  { /* get the address for proper redirection */
    int c;
    #define HELPER(letter) { \
      c = fgetc2(ns); \
      if (c == EOF) { \
        if (verbose && logging) { \
          logger("%s: non-responding at [10], rejected", ip_str); \
        } \
        goto lbl_finish; \
      } \
      if (c != letter) { \
        if (verbose && logging) { \
          logger("%s: invalid http verb, expect GET, rejected", ip_str); \
        } \
        goto lbl_finish; \
      } \
    }
    HELPER('G')
    HELPER('E')
    HELPER('T')
    HELPER(' ')
    #undef HELPER
    for (try = 0; try < ADDR_LEN; try++) {
      c = fgetc2(ns);
      if (c == EOF) {
        if (verbose && logging) {
          logger("%s: non-responding at [10], rejected", ip_str);
        }
        goto lbl_finish;
      }
      if (c == ' ') {
        path[try] = 0;
        break;
      }
      path[try] = c;
    }
    if (try == ADDR_LEN) {
      if (verbose && logging) {
        logger("%s: address of get too big, rejected", ip_str);
      }
      goto lbl_finish;
    }
  }

  { /* search for \r\nHost: */
    char buf[7] = { 0 };

    for (try = 0; try < MAX_TRY; try++) {
      int c, i;
      for (i = 0; i < 6; i++) buf[i] = buf[i + 1];
      c = fgetc2(ns);
      if (c == EOF) {
        if (verbose && logging) {
          logger("%s: non-responding at [20], rejected", ip_str);
        }
        goto lbl_finish;
      }
      buf[6] = c;
      if (!memcmp(&buf[3], "\r\n\r\n", 4)) {
        if (verbose && logging) {
          logger("%s: unexpected end of HTTP header", ip_str);
        }
        goto lbl_finish;
      }
      if (!memcmp(buf, "\r\nHost:", 7)) break;
    }
    if (try >= MAX_TRY) {
      if (verbose && logging) {
        logger("%s: header too big at [30], rejected", ip_str);
      }
      goto lbl_finish;
    }
  }

  { /* test whether the host is in the list of hosts */
    char buf[MAX_LEN] = { 0 };
    int c, i, end;

    for (i = 0; i < MAX_LEN; i++) {
      c = fgetc2(ns);
      if (c == EOF) {
        if (verbose && logging) {
          logger("%s: non-responding at [40], rejected", ip_str);
        }
        goto lbl_finish;
      }
      buf[i] = c;
      if (i >= 1 && buf[i - 1] == '\r' && buf[i] == '\n') break;
    }

    end = i - 1;
    buf[end] = 0;
    listi = 0;

    for (i = listi0; i < listi1; i++) {
      if (strstr(buf, listv[i])) {
        listi = i;
        goto lbl_found_host;
      }
    }
    if (verbose && logging) {
      logger("%s: not in the host list, rejected", ip_str);
    }
    goto lbl_finish;

  lbl_found_host:
    if (verbose && logging) {
      logger("%s: %s: in the hosts list, accepted", ip_str,
             listv[listi]);
    }
  }

  { /* eat all until \r\n\r\n */
    int c, c2, c3;
    char buf[4] = { 0 };

    c2 = fgetc2(ns);
    c3 = fgetc2(ns);
    if (c2 == EOF || c3 == EOF) {
      if (verbose && logging) {
        logger("%s: non-responding at [50], rejected", ip_str);
      }
      goto lbl_finish;
    }
    buf[2] = c2;
    buf[3] = c3;

    if (buf[2] != '\r' || buf[3] != '\n') {
      for (try = 0; try < MAX_TRY; try++) {
        int i;
        for (i = 0; i < 3; i++) buf[i] = buf[i + 1];
        c = fgetc2(ns);
        if (c == EOF) {
          if (verbose && logging) {
            logger("%s: non-responding at [60], rejected", ip_str);
          }
          goto lbl_finish;
        }
        buf[3] = c;
        if (!memcmp(buf, "\r\n\r\n", 4)) break;
      }
      if (try >= MAX_TRY) {
        if (verbose && logging) {
          logger("%s: header too big at [70], rejected", ip_str);
        }
        goto lbl_finish;
      }
    }
  }

  { /* getting here means that all is OK => sends the redirect */
    FILE *out;

    errno = 0;
    out = fdopen(ns, "w");
    if (out == NULL) {
      if (verbose && logging) {
        logger("cannot fdopen: %s", strerror(errno));
      }
      goto lbl_finish;
    }
    fprintf( out
           , "HTTP/1.1 301 Moved Permanently\r\n"
             "Server: http2s\r\n"
             "Location: https://%s%s\r\n"
             "Connection: close\r\n\r\n"
           , listv[listi]
           , path
           );
    fflush(out);
    fclose(out);
    free(arg);
    return NULL;
  }

lbl_finish:
  close(ns);
  new_fail(ip_str);
  free(arg);
  return NULL;
}

/*****************************************************************************/

int main(int argc, char **argv) {
  struct sockaddr_in sin;
  struct thread_arg *targ;
  pthread_t thread;

  /* initialize */
  init(argc, argv);

  /* create server socket */
  errno = 0;
  ss = socket(AF_INET, SOCK_STREAM, 0);
  if (ss == -1) die("cannot create socket: %s", strerror(errno));
  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);
  if (!strcmp(ip,"0.0.0.0")) {
    sin.sin_addr.s_addr = INADDR_ANY;
  }
  else {
    sin.sin_addr.s_addr = inet_addr(ip);
  }
  errno = 0;
  if (bind(ss, (struct sockaddr*)(&sin), sizeof(sin))) {
    die( "cannot bind socket to address %s:%d, %s"
       , ip
       , port
       , strerror(errno)
       );
  }
  errno = 0;
  if ( listen(ss, SOMAXCONN) ) {
    die("cannot listen on socket: %s", strerror(errno));
  }

  /* daemonize if requested */
  if (daemonize) do_daemonize();

  /* dropdown privileges */
  dropdown_privileges();

  /* main loop */
  logger("server started at %s:%d", ip, port);
  for(;;) {
    socklen_t sz_nsin;
    struct sockaddr_in nsin;
    int ns;
    char src_ip[MAX_LEN];
    int src_port;
    int logging;

    memset(&nsin, 0, sizeof(sin));
    sz_nsin = sizeof(nsin);

    errno = 0;
    ns = accept(ss, (struct sockaddr*)(&nsin), &sz_nsin);
    if ( ns == -1 ) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
      if (verbose) {
        logger( "cannot accept incoming connection: %s"
              , strerror(errno)
              );
        continue;
      }
    }

    if (inet_ntop(nsin.sin_family, &nsin.sin_addr,
                  src_ip, MAX_LEN) == NULL)
    {
      strcpy(src_ip, "[unknown ip]");
    }
    src_port = ntohs(nsin.sin_port);
    if (focus_on_ip == NULL) {
      logging = 1;
    }
    else {
      if (!strcmp(src_ip, focus_on_ip)) {
        logging = 1;
      }
      else {
        logging = 0;
      }
    }

    if (verbose && logging) {
      logger("incoming connection from %s:%d", src_ip, src_port);
    }

    targ = malloc(sizeof(struct thread_arg));
    if (targ != NULL) {
      targ->ns = ns;
      targ->logging = logging;
      strcpy(targ->ip_str, src_ip);
      if (pthread_create(&thread, NULL, handle_connection, (void*)targ)) {
        free(targ);
      }
    }
    else {
      logger("cannot malloc: %s", strerror(errno));
    }
  }

  return 0;
}
