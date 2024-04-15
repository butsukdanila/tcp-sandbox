#include "x-logs.h"

const char *
__FILE_NAME__resolve(
  const char  *__file,
  const char **__pfile_path,
  const char **__pfile_name
) {
  if (__file != *__pfile_path) {
    *__pfile_path = __file;
    const char *temp = *__pfile_path;
    while (*temp != '\0') {
      temp++;
    }
    while (temp != __file && *(temp - 1) != '/') {
      temp--;
    }
    *__pfile_name = temp;
  }
  return *__pfile_name;
}