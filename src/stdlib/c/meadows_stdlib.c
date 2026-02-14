#include "meadows_stdlib.h"
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>

int64_t meadows_strlen(const char *s) { return strlen(s); }

char *meadows_strcpy(char *dest, const char *src) { return strcpy(dest, src); }

char *meadows_strcat(char *dest, const char *src) { return strcat(dest, src); }

int32_t meadows_strcmp(const char *s1, const char *s2) {
  return strcmp(s1, s2);
}

int64_t meadows_strncmp(const char *s1, const char *s2, int64_t n) {
  return strncmp(s1, s2, n);
}

char *meadows_strdup(const char *s) { return strdup(s); }

int64_t meadows_strnlen(const char *s, int64_t n) { return strnlen(s, n); }

void *meadows_memcpy(void *dest, const void *src, int64_t n) {
  return memcpy(dest, src, n);
}

int64_t meadows_memcmp(const void *s1, const void *s2, int64_t n) {
  return memcmp(s1, s2, n);
}

char *meadows_alloc_string(int64_t size) { return (char *)malloc(size + 1); }

void meadows_transform_case(char *s, int mode) {
  if (mode == 0) {
    for (int i = 0; s[i]; i++)
      s[i] = toupper(s[i]);
  } else {
    for (int i = 0; s[i]; i++)
      s[i] = tolower(s[i]);
  }
}

void meadows_append_string(char *dest, const char *src) { strcat(dest, src); }

MeadowsFile *meadows_fopen(const char *path, const char *mode) {
  FILE *fp = fopen(path, mode);
  if (!fp)
    return NULL;
  MeadowsFile *mf = (MeadowsFile *)malloc(sizeof(MeadowsFile));
  mf->fp = fp;
  return mf;
}

int meadows_fclose(MeadowsFile *file) {
  int result = fclose(file->fp);
  free(file);
  return result;
}

int64_t meadows_fread(void *buffer, int64_t size, int64_t nmemb,
                      MeadowsFile *file) {
  return fread(buffer, size, nmemb, file->fp);
}

int64_t meadows_fwrite(const void *buffer, int64_t size, int64_t nmemb,
                       MeadowsFile *file) {
  return fwrite(buffer, size, nmemb, file->fp);
}

char *meadows_fgets(char *buffer, int64_t size, MeadowsFile *file) {
  return fgets(buffer, size, file->fp);
}

int meadows_feof(MeadowsFile *file) { return feof(file->fp); }

int meadows_ferror(MeadowsFile *file) { return ferror(file->fp); }

MeadowsFile *meadows_stdin(void) {
  MeadowsFile *mf = (MeadowsFile *)malloc(sizeof(MeadowsFile));
  mf->fp = stdin;
  return mf;
}

MeadowsFile *meadows_stdout(void) {
  MeadowsFile *mf = (MeadowsFile *)malloc(sizeof(MeadowsFile));
  mf->fp = stdout;
  return mf;
}

MeadowsFile *meadows_stderr(void) {
  MeadowsFile *mf = (MeadowsFile *)malloc(sizeof(MeadowsFile));
  mf->fp = stderr;
  return mf;
}

char *meadows_read_file(const char *path) {
  FILE *fp = fopen(path, "rb");
  if (!fp)
    return NULL;
  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);
  rewind(fp);
  char *content = (char *)malloc(size + 1);
  fread(content, 1, size, fp);
  content[size] = '\0';
  fclose(fp);
  return content;
}

int meadows_write_file(const char *path, const char *content) {
  FILE *fp = fopen(path, "w");
  if (!fp)
    return -1;
  fputs(content, fp);
  fclose(fp);
  return 0;
}

int meadows_append_file(const char *path, const char *content) {
  FILE *fp = fopen(path, "a");
  if (!fp)
    return -1;
  fputs(content, fp);
  fclose(fp);
  return 0;
}

int32_t meadows_iabs(int32_t n) { return n < 0 ? -n : n; }

double meadows_fabs(double n) { return fabs(n); }

double meadows_sqrt(double x) { return sqrt(x); }

double meadows_pow(double x, double y) { return pow(x, y); }

double meadows_sin(double x) { return sin(x); }

double meadows_cos(double x) { return cos(x); }

double meadows_tan(double x) { return tan(x); }

double meadows_asin(double x) { return asin(x); }

double meadows_acos(double x) { return acos(x); }

double meadows_atan(double x) { return atan(x); }

double meadows_atan2(double y, double x) { return atan2(y, x); }

double meadows_sinh(double x) { return sinh(x); }

double meadows_cosh(double x) { return cosh(x); }

double meadows_tanh(double x) { return tanh(x); }

double meadows_exp(double x) { return exp(x); }

double meadows_exp2(double x) { return exp2(x); }

double meadows_log(double x) { return log(x); }

double meadows_log10(double x) { return log10(x); }

double meadows_log2(double x) { return log2(x); }

double meadows_cbrt(double x) { return cbrt(x); }

double meadows_hypot(double x, double y) { return hypot(x, y); }

double meadows_floor(double x) { return floor(x); }

double meadows_ceil(double x) { return ceil(x); }

double meadows_round(double x) { return round(x); }

double meadows_trunc(double x) { return trunc(x); }

int32_t meadows_min_i32(int32_t a, int32_t b) { return a < b ? a : b; }

int32_t meadows_max_i32(int32_t a, int32_t b) { return a > b ? a : b; }

int32_t meadows_rand(void) { return rand(); }

void meadows_srand(uint32_t seed) { srand(seed); }

double meadows_min_f64(double a, double b) { return a < b ? a : b; }

double meadows_max_f64(double a, double b) { return a > b ? a : b; }

double meadows_deg2rad(double deg) { return deg * M_PI / 180.0; }

double meadows_rad2deg(double rad) { return rad * 180.0 / M_PI; }

double meadows_fmod(double x, double y) { return fmod(x, y); }

int meadows_isnan(double x) { return isnan(x); }

int meadows_isfinite(double x) { return isfinite(x); }

int meadows_isinf(double x) { return isinf(x); }

double meadows_fma(double x, double y, double z) { return fma(x, y, z); }

double meadows_ldexp(double mant, int exp) { return ldexp(mant, exp); }

double meadows_frexp(double x) {
  int exp;
  double result = frexp(x, &exp);
  (void)exp;
  return result;
}

double meadows_modf(double x) {
  double intpart;
  return modf(x, &intpart);
}

int32_t meadows_dtoi32(double x) { return (int32_t)x; }

int64_t meadows_dtoi64(double x) { return (int64_t)x; }

double meadows_i32tod(int32_t x) { return (double)x; }

double meadows_i64tod(int64_t x) { return (double)x; }

int32_t meadows_index_of(const char *haystack, const char *needle) {
  const char *pos = strstr(haystack, needle);
  if (!pos)
    return -1;
  return (int32_t)(pos - haystack);
}

static const char *strrstr_impl(const char *haystack, const char *needle) {
  if (!*needle)
    return haystack + strlen(haystack);
  const char *result = NULL;
  const char *pos = haystack;
  while ((pos = strstr(pos, needle)) != NULL) {
    result = pos;
    pos++;
  }
  return result;
}

int32_t meadows_last_index_of(const char *haystack, const char *needle) {
  const char *pos = strrstr_impl(haystack, needle);
  if (!pos)
    return -1;
  return (int32_t)(pos - haystack);
}

int32_t meadows_contains(const char *haystack, const char *needle) {
  return strstr(haystack, needle) != NULL ? 1 : 0;
}

int32_t meadows_starts_with(const char *s, const char *prefix) {
  size_t len = strlen(prefix);
  if (strlen(s) < len)
    return 0;
  return strncmp(s, prefix, len) == 0 ? 1 : 0;
}

int32_t meadows_ends_with(const char *s, const char *suffix) {
  size_t len_s = strlen(s);
  size_t len = strlen(suffix);
  if (len_s < len)
    return 0;
  return strcmp(s + len_s - len, suffix) == 0 ? 1 : 0;
}

char *meadows_substring(const char *s, int32_t start, int32_t length) {
  size_t len = strlen(s);
  if (start >= (int32_t)len || length <= 0)
    return strdup("");
  if (start + length > (int32_t)len)
    length = len - start;
  char *result = (char *)malloc(length + 1);
  memcpy(result, s + start, length);
  result[length] = '\0';
  return result;
}

char *meadows_trim(const char *s) {
  while (isspace((unsigned char)*s))
    s++;
  if (!*s)
    return strdup("");
  const char *end = s + strlen(s) - 1;
  while (end > s && isspace((unsigned char)*end))
    end--;
  size_t len = end - s + 1;
  char *result = (char *)malloc(len + 1);
  memcpy(result, s, len);
  result[len] = '\0';
  return result;
}

char *meadows_trim_left(const char *s) {
  while (isspace((unsigned char)*s))
    s++;
  return strdup(s);
}

char *meadows_trim_right(const char *s) {
  const char *end = s + strlen(s) - 1;
  while (end > s && isspace((unsigned char)*end))
    end--;
  size_t len = end - s + 1;
  char *result = (char *)malloc(len + 1);
  memcpy(result, s, len);
  result[len] = '\0';
  return result;
}

char *meadows_reverse(const char *s) {
  size_t len = strlen(s);
  char *result = (char *)malloc(len + 1);
  for (size_t i = 0; i < len; i++)
    result[i] = s[len - 1 - i];
  result[len] = '\0';
  return result;
}

char *meadows_replace(const char *s, const char *old_str, const char *to) {
  if (!old_str[0])
    return strdup(s);
  char *result = NULL;
  const char *start = s;
  const char *match = NULL;
  size_t from_len = strlen(old_str);
  size_t to_len = strlen(to);

  while ((match = strstr(start, old_str)) != NULL) {
    size_t prefix_len = match - s;
    size_t suffix_len = strlen(match + from_len);
    size_t new_len = prefix_len + to_len + suffix_len;
    char *new_result = (char *)malloc(new_len + 1);
    memcpy(new_result, s, prefix_len);
    memcpy(new_result + prefix_len, to, to_len);
    memcpy(new_result + prefix_len + to_len, match + from_len, suffix_len);
    new_result[new_len] = '\0';
    free(result);
    result = new_result;
    start = result + prefix_len + to_len;
    s = result;
  }

  if (!result)
    return strdup(s);
  return result;
}

char *meadows_to_upper(const char *s) {
  char *result = strdup(s);
  for (int i = 0; result[i]; i++)
    result[i] = toupper((unsigned char)result[i]);
  return result;
}

char *meadows_to_lower(const char *s) {
  char *result = strdup(s);
  for (int i = 0; result[i]; i++)
    result[i] = tolower((unsigned char)result[i]);
  return result;
}

char *meadows_format_int(int32_t n) {
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%d", n);
  return strdup(buffer);
}

char *meadows_format_float(double n, int32_t decimals) {
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "%.*f", decimals, n);
  return strdup(buffer);
}

int32_t meadows_fseek(MeadowsFile *file, int64_t offset, int32_t whence) {
  return fseek(file->fp, offset, whence);
}

int64_t meadows_ftell(MeadowsFile *file) { return ftell(file->fp); }

void meadows_rewind(MeadowsFile *file) { rewind(file->fp); }

int32_t meadows_fflush(MeadowsFile *file) { return fflush(file->fp); }

int32_t meadows_remove_file(const char *path) { return remove(path); }

int32_t meadows_rename_file(const char *old_path, const char *new_path) {
  return rename(old_path, new_path);
}

int32_t meadows_file_exists(const char *path) {
  FILE *fp = fopen(path, "r");
  if (fp) {
    fclose(fp);
    return 1;
  }
  return 0;
}

int64_t meadows_array_create(int32_t size) {
  int32_t *arr = (int32_t *)malloc(size * sizeof(int32_t));
  return (int64_t)arr;
}

int64_t meadows_array_copy(int64_t src, int64_t dest, int32_t size) {
  int32_t *src_ptr = (int32_t *)src;
  int32_t *dest_ptr = (int32_t *)dest;
  memcpy(dest_ptr, src_ptr, size * sizeof(int32_t));
  return dest;
}

void meadows_array_fill(int64_t arr, int32_t value, int32_t size) {
  int32_t *arr_ptr = (int32_t *)arr;
  for (int32_t i = 0; i < size; i++)
    arr_ptr[i] = value;
}

int32_t meadows_array_index_of(int64_t arr, int32_t value, int32_t size) {
  int32_t *arr_ptr = (int32_t *)arr;
  for (int32_t i = 0; i < size; i++)
    if (arr_ptr[i] == value)
      return i;
  return -1;
}

int32_t meadows_array_contains(int64_t arr, int32_t value, int32_t size) {
  return meadows_array_index_of(arr, value, size) != -1 ? 1 : 0;
}

void meadows_array_sort(int64_t arr, int32_t size) {
  int32_t *arr_ptr = (int32_t *)arr;
  for (int32_t i = 0; i < size - 1; i++)
    for (int32_t j = 0; j < size - i - 1; j++)
      if (arr_ptr[j] > arr_ptr[j + 1]) {
        int32_t temp = arr_ptr[j];
        arr_ptr[j] = arr_ptr[j + 1];
        arr_ptr[j + 1] = temp;
      }
}

int64_t meadows_time(void) { return time(NULL); }

void meadows_sleep(int64_t milliseconds) { usleep(milliseconds * 1000); }

int64_t meadows_clock(void) { return clock(); }

char *meadows_getenv(const char *name) {
  const char *value = getenv(name);
  return value ? strdup(value) : NULL;
}

int32_t meadows_setenv(const char *name, const char *value) {
  return setenv(name, value, 1) == 0 ? 0 : -1;
}

void meadows_exit(int32_t code) { exit(code); }

int64_t meadows_opendir(const char *path) {
  DIR *dir = opendir(path);
  if (!dir)
    return 0;
  return (int64_t)(intptr_t)dir;
}

char *meadows_readdir(int64_t dir_ptr) {
  DIR *dir = (DIR *)(intptr_t)dir_ptr;
  if (!dir)
    return strdup("");
  struct dirent *entry = readdir(dir);
  if (!entry)
    return strdup("");
  return strdup(entry->d_name);
}

int32_t meadows_closedir(int64_t dir_ptr) {
  DIR *dir = (DIR *)(intptr_t)dir_ptr;
  if (!dir)
    return -1;
  return closedir(dir);
}

int32_t meadows_mkdir(const char *path, int32_t mode) {
  return mkdir(path, (mode_t)mode);
}

int32_t meadows_rmdir(const char *path) { return rmdir(path); }

int32_t meadows_is_directory(const char *path) {
  struct stat st;
  if (stat(path, &st) != 0)
    return 0;
  return S_ISDIR(st.st_mode);
}

static int global_argc = 0;
static char **global_argv = NULL;

int32_t meadows_args(void) { return global_argc; }

char *meadows_getarg(int32_t n) {
  if (n < 0 || n >= global_argc)
    return strdup("");
  return strdup(global_argv[n]);
}

void meadows_set_args(int argc, char *argv[]) {
  global_argc = argc;
  global_argv = argv;
}
