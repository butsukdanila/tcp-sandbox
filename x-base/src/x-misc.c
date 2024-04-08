#include "x-misc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <sys/fcntl.h>

static size_t
_printx(char * dst, size_t dst_sz, const u08 * src, size_t src_sz) {
  static const char __hex[] = "0123456789abcdef";
  for (size_t i = 0; i < dst_sz; ++i) {
    if (i < src_sz) {
      u08 c = *src++;
      *dst++ = __hex[c >> 0x04];
      *dst++ = __hex[c &  0x0f];
    } else {
      *dst++ = '-';
      *dst++ = '-';
    }
    *dst++ = ' ';
  }
  return dst_sz * 3;
}

static size_t
_printc(char * dst, size_t dst_sz, const u08 * src, size_t src_sz) {
  for (size_t i = 0; i < dst_sz; ++i) {
    if (i < src_sz) {
      u08 c = *src++;
      *dst++ = isprint(c) ? (char)c : '.';
    } else {
      *dst++ = ' ';
    }
  }
  return dst_sz;
}

char *
x_hexdump(const void * src, size_t src_sz) {
  size_t line_max = 0x10;
  size_t line_len = (8) + (2) + (line_max * 3) + (1) + (1) + (line_max) + (1) + (1);
  size_t line_dev = src_sz / line_max;
  size_t line_rem = src_sz % line_max;
  size_t line_cnt = line_dev + (line_rem ? 1 : 0);
  size_t line_cur = 0;
  char * dst = malloc(line_len * line_cnt);
  char * tmp_dst = dst;
  u08 *  tmp_src = (u08 *)src;
  for (size_t i = 0; i < line_cnt; ++i) {
    line_cur = i + 1 != line_cnt ? line_max : line_max - line_rem;
    tmp_dst += sprintf(tmp_dst, "%08lx  ", i * line_max);
    tmp_dst += _printx(tmp_dst, line_max, tmp_src, line_cur);
    tmp_dst += sprintf(tmp_dst, " |");
    tmp_dst += _printc(tmp_dst, line_max, tmp_src, line_cur);
    tmp_dst += sprintf(tmp_dst, "|%c", i + 1 != line_cnt ? '\n' : '\0');
    tmp_src += line_cur;
  }
  return dst;
}

bool 
x_fd_has_flag(int fd, int flag) {
  int fdf = fcntl(fd, F_GETFD);
  if (fdf < 0) {
    return false;
  }
  return fdf & flag;
}

int
x_fd_add_flag(int fd, int flag) {
  int fdf = fcntl(fd, F_GETFD);
  if (fdf < 0) {
    return -1;
  }
  return fcntl(fd, F_SETFD, fdf | flag);
}

int
x_fd_del_flag(int fd, int flag) {
  int fdf = fcntl(fd, F_GETFD);
  if (fdf < 0) {
    return -1;
  }
  return fcntl(fd, F_SETFD, fdf & ~flag);
}