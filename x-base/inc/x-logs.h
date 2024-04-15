#ifndef __X_LOGS_H__
#define __X_LOGS_H__

#include <stdio.h>  // fprintf, stdout, stderr
#include <errno.h>  // errno
#include <string.h> // strerror

const char *
__FILE_NAME__resolve(
  const char  *__file,
  const char **__pfile_path,
  const char **__pfile_name
);

static __thread const char *__FILE_NAME__tl_path __attribute__((unused));
static __thread const char *__FILE_NAME__tl_name __attribute__((unused));

#define __FILE_NAME__ \
  __FILE_NAME__resolve(__FILE__, &__FILE_NAME__tl_path, &__FILE_NAME__tl_name)

#define logi(_fmt, ...) fprintf(stderr, "[inf][%s:%d]: " _fmt "\n", __FILE_NAME__, __LINE__, ##__VA_ARGS__)
#define loge(_fmt, ...) fprintf(stderr, "[err][%s:%d]: " _fmt "\n", __FILE_NAME__, __LINE__, ##__VA_ARGS__)

#define logi_cycle(_fmt, ...) fprintf(stdout, "\033[A\33[2K\r[inf][%s:%d]: " _fmt "\n", __FILE_NAME__, __LINE__, ##__VA_ARGS__)
#define loge_errno(_dsc)      loge(_dsc ": %s (%d)", strerror(errno), errno)

#endif//__X_LOGS_H__