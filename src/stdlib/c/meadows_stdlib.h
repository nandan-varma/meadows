#ifndef MEADOWS_STDLIB_H
#define MEADOWS_STDLIB_H

#include <dirent.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

struct MeadowsFile {
  FILE *fp;
};

typedef struct MeadowsFile MeadowsFile;

#ifdef __cplusplus
extern "C" {
#endif

int64_t meadows_strlen(const char *s);
char *meadows_strcpy(char *dest, const char *src);
char *meadows_strcat(char *dest, const char *src);
int32_t meadows_strcmp(const char *s1, const char *s2);
int64_t meadows_strncmp(const char *s1, const char *s2, int64_t n);
char *meadows_strdup(const char *s);
int64_t meadows_strnlen(const char *s, int64_t n);
void *meadows_memcpy(void *dest, const void *src, int64_t n);
int64_t meadows_memcmp(const void *s1, const void *s2, int64_t n);
char *meadows_alloc_string(int64_t size);
void meadows_transform_case(char *s, int mode);
void meadows_append_string(char *dest, const char *src);

MeadowsFile *meadows_fopen(const char *path, const char *mode);
int meadows_fclose(MeadowsFile *file);
int64_t meadows_fread(void *buffer, int64_t size, int64_t nmemb,
                      MeadowsFile *file);
int64_t meadows_fwrite(const void *buffer, int64_t size, int64_t nmemb,
                       MeadowsFile *file);
char *meadows_fgets(char *buffer, int64_t size, MeadowsFile *file);
int meadows_feof(MeadowsFile *file);
int meadows_ferror(MeadowsFile *file);
MeadowsFile *meadows_stdin(void);
MeadowsFile *meadows_stdout(void);
MeadowsFile *meadows_stderr(void);
char *meadows_read_file(const char *path);
int meadows_write_file(const char *path, const char *content);
int meadows_append_file(const char *path, const char *content);

int32_t meadows_iabs(int32_t n);
double meadows_fabs(double n);
double meadows_sqrt(double x);
double meadows_pow(double x, double y);
double meadows_sin(double x);
double meadows_cos(double x);
double meadows_tan(double x);
double meadows_asin(double x);
double meadows_acos(double x);
double meadows_atan(double x);
double meadows_atan2(double y, double x);
double meadows_sinh(double x);
double meadows_cosh(double x);
double meadows_tanh(double x);
double meadows_exp(double x);
double meadows_exp2(double x);
double meadows_log(double x);
double meadows_log10(double x);
double meadows_log2(double x);
double meadows_cbrt(double x);
double meadows_hypot(double x, double y);
double meadows_floor(double x);
double meadows_ceil(double x);
double meadows_round(double x);
double meadows_trunc(double x);
int32_t meadows_min_i32(int32_t a, int32_t b);
int32_t meadows_max_i32(int32_t a, int32_t b);
int32_t meadows_rand(void);
void meadows_srand(uint32_t seed);
double meadows_min_f64(double a, double b);
double meadows_max_f64(double a, double b);
double meadows_deg2rad(double deg);
double meadows_rad2deg(double rad);
double meadows_fmod(double x, double y);
int meadows_isnan(double x);
int meadows_isfinite(double x);
int meadows_isinf(double x);
double meadows_fma(double x, double y, double z);
double meadows_ldexp(double mant, int exp);
double meadows_frexp(double x);
double meadows_modf(double x);
int32_t meadows_dtoi32(double x);
int64_t meadows_dtoi64(double x);
double meadows_i32tod(int32_t x);
double meadows_i64tod(int64_t x);
int32_t meadows_index_of(const char *haystack, const char *needle);
int32_t meadows_last_index_of(const char *haystack, const char *needle);
int32_t meadows_contains(const char *haystack, const char *needle);
int32_t meadows_starts_with(const char *s, const char *prefix);
int32_t meadows_ends_with(const char *s, const char *suffix);
char *meadows_substring(const char *s, int32_t start, int32_t length);
char *meadows_trim(const char *s);
char *meadows_trim_left(const char *s);
char *meadows_trim_right(const char *s);
char *meadows_reverse(const char *s);
char *meadows_replace(const char *s, const char *from, const char *to);
char *meadows_to_upper(const char *s);
char *meadows_to_lower(const char *s);
char *meadows_format_int(int32_t n);
char *meadows_format_float(double n, int32_t decimals);
int32_t meadows_fseek(MeadowsFile *file, int64_t offset, int32_t whence);
int64_t meadows_ftell(MeadowsFile *file);
void meadows_rewind(MeadowsFile *file);
int32_t meadows_fflush(MeadowsFile *file);
int32_t meadows_remove_file(const char *path);
int32_t meadows_rename_file(const char *old_path, const char *new_path);
int32_t meadows_file_exists(const char *path);

int64_t meadows_opendir(const char *path);
char *meadows_readdir(int64_t dir_ptr);
int32_t meadows_closedir(int64_t dir_ptr);
int32_t meadows_mkdir(const char *path, int32_t mode);
int32_t meadows_rmdir(const char *path);
int32_t meadows_is_directory(const char *path);

int32_t meadows_args(void);
char *meadows_getarg(int32_t n);
void meadows_set_args(int argc, char *argv[]);
int64_t meadows_array_create(int32_t size);
int64_t meadows_array_copy(int64_t src, int64_t dest, int32_t size);
void meadows_array_fill(int64_t arr, int32_t value, int32_t size);
int32_t meadows_array_index_of(int64_t arr, int32_t value, int32_t size);
int32_t meadows_array_contains(int64_t arr, int32_t value, int32_t size);
void meadows_array_sort(int64_t arr, int32_t size);
int64_t meadows_time(void);
void meadows_sleep(int64_t milliseconds);
int64_t meadows_clock(void);
char *meadows_getenv(const char *name);
int32_t meadows_setenv(const char *name, const char *value);
void meadows_exit(int32_t code);

#ifdef __cplusplus
}
#endif

#endif
