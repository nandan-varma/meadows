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

typedef struct {
  int32_t *data;
  int32_t length;
  int32_t capacity;
} VecI32;

int64_t meadows_vec_create(int32_t capacity) {
  VecI32 *vec = (VecI32 *)malloc(sizeof(VecI32));
  vec->capacity = capacity > 0 ? capacity : 4;
  vec->length = 0;
  vec->data = (int32_t *)malloc(sizeof(int32_t) * vec->capacity);
  return (int64_t)vec;
}

void meadows_vec_free(int64_t vec_ptr) {
  VecI32 *vec = (VecI32 *)vec_ptr;
  if (vec) {
    free(vec->data);
    free(vec);
  }
}

int32_t meadows_vec_len(int64_t vec_ptr) {
  VecI32 *vec = (VecI32 *)vec_ptr;
  return vec ? vec->length : 0;
}

int64_t meadows_vec_push(int64_t vec_ptr, int32_t value) {
  VecI32 *vec = (VecI32 *)vec_ptr;
  if (!vec)
    return vec_ptr;

  if (vec->length >= vec->capacity) {
    vec->capacity *= 2;
    vec->data = (int32_t *)realloc(vec->data, sizeof(int32_t) * vec->capacity);
  }
  vec->data[vec->length++] = value;
  return vec_ptr;
}

int32_t meadows_vec_get(int64_t vec_ptr, int32_t index) {
  VecI32 *vec = (VecI32 *)vec_ptr;
  if (!vec || index < 0 || index >= vec->length)
    return 0;
  return vec->data[index];
}

void meadows_vec_set(int64_t vec_ptr, int32_t index, int32_t value) {
  VecI32 *vec = (VecI32 *)vec_ptr;
  if (!vec || index < 0 || index >= vec->length)
    return;
  vec->data[index] = value;
}

typedef struct {
  char **keys;
  int32_t *values;
  int32_t length;
  int32_t capacity;
} HashMap;

int32_t meadows_hashmap_hash(const char *key) {
  int32_t hash = 5381;
  int c;
  while ((c = *key++)) {
    hash = ((hash << 5) + hash) + c;
  }
  return hash;
}

int64_t meadows_hashmap_create(int32_t capacity) {
  HashMap *map = (HashMap *)malloc(sizeof(HashMap));
  map->capacity = capacity > 0 ? capacity : 16;
  map->length = 0;
  map->keys = (char **)calloc(map->capacity, sizeof(char *));
  map->values = (int32_t *)calloc(map->capacity, sizeof(int32_t));
  return (int64_t)map;
}

void meadows_hashmap_free(int64_t map_ptr) {
  HashMap *map = (HashMap *)map_ptr;
  if (!map)
    return;
  for (int32_t i = 0; i < map->capacity; i++) {
    if (map->keys[i])
      free(map->keys[i]);
  }
  free(map->keys);
  free(map->values);
  free(map);
}

int32_t meadows_hashmap_len(int64_t map_ptr) {
  HashMap *map = (HashMap *)map_ptr;
  return map ? map->length : 0;
}

static int32_t meadows_hashmap_find(HashMap *map, const char *key) {
  int32_t index = meadows_hashmap_hash(key) % map->capacity;
  for (int32_t i = 0; i < map->capacity; i++) {
    int32_t idx = (index + i) % map->capacity;
    if (!map->keys[idx])
      return -1;
    if (strcmp(map->keys[idx], key) == 0)
      return idx;
  }
  return -1;
}

int64_t meadows_hashmap_put(int64_t map_ptr, const char *key, int32_t value) {
  HashMap *map = (HashMap *)map_ptr;
  if (!map)
    return map_ptr;

  int32_t idx = meadows_hashmap_find(map, key);
  if (idx >= 0) {
    map->values[idx] = value;
    return map_ptr;
  }

  if (map->length >= map->capacity / 2) {
    map->capacity *= 2;
    map->keys = (char **)realloc(map->keys, sizeof(char *) * map->capacity);
    map->values =
        (int32_t *)realloc(map->values, sizeof(int32_t) * map->capacity);
  }

  idx = meadows_hashmap_hash(key) % map->capacity;
  while (map->keys[idx]) {
    idx = (idx + 1) % map->capacity;
  }
  map->keys[idx] = strdup(key);
  map->values[idx] = value;
  map->length++;
  return map_ptr;
}

int32_t meadows_hashmap_get(int64_t map_ptr, const char *key) {
  HashMap *map = (HashMap *)map_ptr;
  if (!map)
    return 0;
  int32_t idx = meadows_hashmap_find(map, key);
  return idx >= 0 ? map->values[idx] : 0;
}

int32_t meadows_hashmap_has(int64_t map_ptr, const char *key) {
  HashMap *map = (HashMap *)map_ptr;
  if (!map)
    return 0;
  return meadows_hashmap_find(map, key) >= 0 ? 1 : 0;
}

int64_t meadows_hashmap_remove(int64_t map_ptr, const char *key) {
  HashMap *map = (HashMap *)map_ptr;
  if (!map)
    return map_ptr;

  int32_t idx = meadows_hashmap_find(map, key);
  if (idx < 0)
    return map_ptr;

  free(map->keys[idx]);
  map->keys[idx] = NULL;
  map->values[idx] = 0;
  map->length--;
  return map_ptr;
}

int64_t meadows_hashmap_keys(int64_t map_ptr) {
  HashMap *map = (HashMap *)map_ptr;
  int64_t keys_vec = meadows_vec_create(map ? map->length : 0);
  if (!map)
    return keys_vec;

  for (int32_t i = 0; i < map->capacity; i++) {
    if (map->keys[i]) {
      meadows_vec_push(keys_vec, i);
    }
  }
  return keys_vec;
}

typedef struct {
  int32_t *values;
  int32_t length;
  int32_t capacity;
} HashSet;

int64_t meadows_hashset_create(int32_t capacity) {
  HashSet *set = (HashSet *)malloc(sizeof(HashSet));
  set->capacity = capacity > 0 ? capacity : 16;
  set->length = 0;
  set->values = (int32_t *)calloc(set->capacity, sizeof(int32_t));
  return (int64_t)set;
}

void meadows_hashset_free(int64_t set_ptr) {
  HashSet *set = (HashSet *)set_ptr;
  if (set) {
    free(set->values);
    free(set);
  }
}

int32_t meadows_hashset_len(int64_t set_ptr) {
  HashSet *set = (HashSet *)set_ptr;
  return set ? set->length : 0;
}

static int32_t meadows_hashset_index(HashSet *set, int32_t value) {
  int32_t index = ((value % set->capacity) + set->capacity) % set->capacity;
  for (int32_t i = 0; i < set->capacity; i++) {
    int32_t idx = (index + i) % set->capacity;
    if (set->values[idx] == value)
      return idx;
    if (set->values[idx] == 0)
      return -1;
  }
  return -1;
}

int64_t meadows_hashset_add(int64_t set_ptr, int32_t value) {
  HashSet *set = (HashSet *)set_ptr;
  if (!set)
    return set_ptr;

  if (meadows_hashset_has(set_ptr, value))
    return set_ptr;

  if (set->length >= set->capacity / 2) {
    set->capacity *= 2;
    set->values =
        (int32_t *)realloc(set->values, sizeof(int32_t) * set->capacity);
  }

  int32_t index = ((value % set->capacity) + set->capacity) % set->capacity;
  while (set->values[index]) {
    index = (index + 1) % set->capacity;
  }
  set->values[index] = value;
  set->length++;
  return set_ptr;
}

int32_t meadows_hashset_has(int64_t set_ptr, int32_t value) {
  HashSet *set = (HashSet *)set_ptr;
  if (!set)
    return 0;
  return meadows_hashset_index(set, value) >= 0 ? 1 : 0;
}

int64_t meadows_hashset_remove(int64_t set_ptr, int32_t value) {
  HashSet *set = (HashSet *)set_ptr;
  if (!set)
    return set_ptr;

  int32_t idx = meadows_hashset_index(set, value);
  if (idx < 0)
    return set_ptr;

  set->values[idx] = 0;
  set->length--;
  return set_ptr;
}
