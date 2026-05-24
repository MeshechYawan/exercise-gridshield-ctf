#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define XKEY 0x5a

static const unsigned char enc_pass[] = {0x09, 0x2e, 0x28, 0x6a, 0x34, 0x3d, 0x79, 0x18, 0x31, 0x2a, 0x68, 0x6a, 0x68, 0x6e, 0x00};

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

static const char *arg_value(int argc, char **argv, const char *key) {
  for (int i = 1; i + 1 < argc; i++) {
    if (strcmp(argv[i], key) == 0) {
      return argv[i + 1];
    }
  }
  return NULL;
}

int main(int argc, char **argv) {
  const char *profile = arg_value(argc, argv, "--profile");
  const char *host = arg_value(argc, argv, "--host");
  const char *user = arg_value(argc, argv, "--user");
  const char *netrc = arg_value(argc, argv, "--netrc");
  char pass[32];
  FILE *f;

  if (!profile || strcmp(profile, "cbfs01") != 0 || !host || !user || !netrc) {
    return 2;
  }

  decode(enc_pass, pass, sizeof(pass));
  f = fopen(netrc, "w");
  if (!f) {
    burn(pass, sizeof(pass));
    return 3;
  }
  fprintf(f, "machine %s\nlogin %s\npassword %s\n", host, user, pass);
  fclose(f);
  chmod(netrc, 0600);
  burn(pass, sizeof(pass));
  return 0;
}
