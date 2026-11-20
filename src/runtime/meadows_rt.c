#include "meadows_rt.h"

#include <math.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Limits.h"

#define IMMORTAL UINT32_MAX

typedef struct mrt_array {
  uint32_t refs;
  uint32_t length;
  uint32_t capacity;
  mrt_value* items;
} mrt_array;

typedef struct mrt_object {
  uint32_t refs;
  uint32_t reserved;
  const mrt_shape* shape;
  mrt_value fields[];
} mrt_object;

static const char* const TYPE_NAMES[] = {"integer", "float",  "string",
                                         "array",   "object", "function"};
static const char* const TYPE_DESCRIPTIONS[] = {
    "an integer", "a float", "a string", "an array", "an object", "a function"};

static void (*program_entry)(void);
static uintptr_t stack_limit;
static uint32_t call_depth;

static mrt_value* graveyard;
static size_t graveyard_length;
static size_t graveyard_capacity;
static int draining;

_Noreturn static void fail(const mrt_site* site, const char* format, ...) {
  va_list args;
  va_start(args, format);
  va_list probe;
  va_copy(probe, args);
  int length = vsnprintf(NULL, 0, format, probe);
  va_end(probe);
  char* message = malloc((size_t)length + 1);
  if (message) {
    vsnprintf(message, (size_t)length + 1, format, args);
  }
  va_end(args);

  fflush(stdout);
  if (site) {
    fprintf(stderr, "RuntimeError: %s\n  at %s:%u:%u\n",
            message ? message : "Out of memory", site->file, site->line,
            site->column);
  } else {
    fprintf(stderr, "RuntimeError: %s\n", message ? message : "Out of memory");
  }
  exit(1);
}

static void* allocate(size_t size) {
  void* memory = malloc(size ? size : 1);
  if (!memory) {
    fail(NULL, "Out of memory");
  }
  return memory;
}

static const char* plural(uint32_t count, char* buffer, size_t size) {
  snprintf(buffer, size, "%u argument%s", count, count == 1 ? "" : "s");
  return buffer;
}

static int is_heap(uint32_t tag) {
  return tag == MRT_STRING || tag == MRT_ARRAY || tag == MRT_OBJECT;
}

static int is_number(const mrt_value* value) {
  return value->tag <= MRT_FLOAT;
}

static double as_double(const mrt_value* value) {
  return value->tag == MRT_INT ? (double)value->as.integer : value->as.real;
}

static void set_int(mrt_value* out, int32_t value) {
  out->tag = MRT_INT;
  out->reserved = 0;
  out->as.integer = value;
}

static void set_float(mrt_value* out, double value) {
  out->tag = MRT_FLOAT;
  out->reserved = 0;
  out->as.real = value;
}

static mrt_string* new_string(uint32_t length) {
  mrt_string* string = allocate(sizeof(mrt_string) + length + 1);
  string->refs = 1;
  string->length = length;
  string->data[length] = '\0';
  return string;
}

static void set_string(mrt_value* out, mrt_string* string) {
  out->tag = MRT_STRING;
  out->reserved = 0;
  out->as.pointer = string;
}

static void bury(mrt_value dead) {
  if (graveyard_length == graveyard_capacity) {
    graveyard_capacity = graveyard_capacity ? graveyard_capacity * 2 : 64;
    graveyard = realloc(graveyard, graveyard_capacity * sizeof(mrt_value));
    if (!graveyard) {
      fail(NULL, "Out of memory");
    }
  }
  graveyard[graveyard_length++] = dead;
}

static void drop(const mrt_value* value) {
  if (!is_heap(value->tag)) {
    return;
  }
  uint32_t* refs = value->as.pointer;
  if (*refs != IMMORTAL && --*refs == 0) {
    bury(*value);
  }
}

static void destroy(mrt_value dead) {
  switch (dead.tag) {
  case MRT_STRING:
    free(dead.as.pointer);
    break;
  case MRT_ARRAY: {
    mrt_array* array = dead.as.pointer;
    for (uint32_t i = 0; i < array->length; ++i) {
      drop(&array->items[i]);
    }
    free(array->items);
    free(array);
    break;
  }
  case MRT_OBJECT: {
    mrt_object* object = dead.as.pointer;
    for (uint32_t i = 0; i < object->shape->count; ++i) {
      drop(&object->fields[i]);
    }
    free(object);
    break;
  }
  default:
    break;
  }
}

void mrt_release(mrt_value* value) {
  drop(value);
  if (draining) {
    return;
  }
  draining = 1;
  while (graveyard_length > 0) {
    destroy(graveyard[--graveyard_length]);
  }
  draining = 0;
}

void mrt_copy(mrt_value* out, const mrt_value* value) {
  *out = *value;
  if (is_heap(value->tag)) {
    uint32_t* refs = value->as.pointer;
    if (*refs != IMMORTAL) {
      ++*refs;
    }
  }
}

void mrt_store(mrt_value* target, mrt_value* value) {
  mrt_value old = *target;
  *target = *value;
  mrt_release(&old);
}

static void* thread_main(void* unused) {
  (void)unused;
  char marker;
  stack_limit =
      (uintptr_t)&marker - (MEADOWS_STACK_SIZE - MEADOWS_STACK_RESERVE);
  program_entry();
  return NULL;
}

int mrt_run(void (*entry)(void)) {
  program_entry = entry;
  pthread_attr_t attributes;
  pthread_attr_init(&attributes);
  pthread_attr_setstacksize(&attributes, MEADOWS_STACK_SIZE);
  pthread_t thread;
  int status = pthread_create(&thread, &attributes, thread_main, NULL);
  pthread_attr_destroy(&attributes);
  if (status != 0) {
    fputs("RuntimeError: cannot allocate the program stack\n", stderr);
    return 1;
  }
  pthread_join(thread, NULL);
  fflush(stdout);
  return 0;
}

void mrt_make_int(mrt_value* out, int32_t value) {
  set_int(out, value);
}

void mrt_make_float(mrt_value* out, double value) {
  set_float(out, value);
}

void mrt_make_string(mrt_value* out, const mrt_string* literal) {
  out->tag = MRT_STRING;
  out->reserved = 0;
  out->as.pointer = (void*)literal;
}

void mrt_make_function(mrt_value* out, const mrt_function* function) {
  out->tag = MRT_FUNCTION;
  out->reserved = 0;
  out->as.pointer = (void*)function;
}

void mrt_make_array(mrt_value* out, mrt_value* items, uint32_t count) {
  mrt_array* array = allocate(sizeof(mrt_array));
  array->refs = 1;
  array->length = count;
  array->capacity = count;
  array->items = count ? allocate(count * sizeof(mrt_value)) : NULL;
  if (count) {
    memcpy(array->items, items, count * sizeof(mrt_value));
  }
  out->tag = MRT_ARRAY;
  out->reserved = 0;
  out->as.pointer = array;
}

void mrt_make_object(mrt_value* out, const mrt_shape* shape,
                     mrt_value* fields) {
  mrt_object* object =
      allocate(sizeof(mrt_object) + shape->count * sizeof(mrt_value));
  object->refs = 1;
  object->shape = shape;
  if (shape->count) {
    memcpy(object->fields, fields, shape->count * sizeof(mrt_value));
  }
  out->tag = MRT_OBJECT;
  out->reserved = 0;
  out->as.pointer = object;
}

static int equals(const mrt_value* a, const mrt_value* b) {
  if (is_number(a) && is_number(b)) {
    return as_double(a) == as_double(b);
  }
  if (a->tag != b->tag) {
    return 0;
  }
  if (a->tag == MRT_STRING) {
    const mrt_string* x = a->as.pointer;
    const mrt_string* y = b->as.pointer;
    return x->length == y->length && memcmp(x->data, y->data, x->length) == 0;
  }
  return a->as.pointer == b->as.pointer;
}

void mrt_eq(mrt_value* out, const mrt_value* a, const mrt_value* b) {
  set_int(out, equals(a, b));
}

void mrt_ne(mrt_value* out, const mrt_value* a, const mrt_value* b) {
  set_int(out, !equals(a, b));
}

_Noreturn static void unsupported(const mrt_site* site, const char* op,
                                  const mrt_value* a, const mrt_value* b) {
  fail(site, "Operator '%s' cannot be applied to %s and %s", op,
       TYPE_NAMES[a->tag], TYPE_NAMES[b->tag]);
}

static uint32_t text_of(const mrt_value* value, char* buffer,
                        const char** data) {
  if (value->tag == MRT_STRING) {
    const mrt_string* string = value->as.pointer;
    *data = string->data;
    return string->length;
  }
  int length = value->tag == MRT_INT
                   ? snprintf(buffer, 32, "%d", value->as.integer)
                   : snprintf(buffer, 32, "%g", value->as.real);
  *data = buffer;
  return (uint32_t)length;
}

static void concatenate(mrt_value* out, const mrt_value* a, const mrt_value* b,
                        const mrt_site* site) {
  if ((a->tag != MRT_STRING && !is_number(a)) ||
      (b->tag != MRT_STRING && !is_number(b))) {
    unsupported(site, "+", a, b);
  }
  char left_buffer[32];
  char right_buffer[32];
  const char* left;
  const char* right;
  uint32_t left_length = text_of(a, left_buffer, &left);
  uint32_t right_length = text_of(b, right_buffer, &right);
  if ((uint64_t)left_length + right_length > MEADOWS_MAX_STRING_LENGTH) {
    fail(site, "String exceeds the maximum length");
  }
  mrt_string* joined = new_string(left_length + right_length);
  memcpy(joined->data, left, left_length);
  memcpy(joined->data + left_length, right, right_length);
  set_string(out, joined);
}

static int32_t wrap(int64_t value) {
  return (int32_t)(uint32_t)value;
}

void mrt_add(mrt_value* out, const mrt_value* a, const mrt_value* b,
             const mrt_site* site) {
  if (a->tag == MRT_STRING || b->tag == MRT_STRING) {
    concatenate(out, a, b, site);
  } else if (!is_number(a) || !is_number(b)) {
    unsupported(site, "+", a, b);
  } else if (a->tag == MRT_INT && b->tag == MRT_INT) {
    set_int(out, wrap((int64_t)a->as.integer + b->as.integer));
  } else {
    set_float(out, as_double(a) + as_double(b));
  }
}

void mrt_sub(mrt_value* out, const mrt_value* a, const mrt_value* b,
             const mrt_site* site) {
  if (!is_number(a) || !is_number(b)) {
    unsupported(site, "-", a, b);
  }
  if (a->tag == MRT_INT && b->tag == MRT_INT) {
    set_int(out, wrap((int64_t)a->as.integer - b->as.integer));
  } else {
    set_float(out, as_double(a) - as_double(b));
  }
}

void mrt_mul(mrt_value* out, const mrt_value* a, const mrt_value* b,
             const mrt_site* site) {
  if (!is_number(a) || !is_number(b)) {
    unsupported(site, "*", a, b);
  }
  if (a->tag == MRT_INT && b->tag == MRT_INT) {
    set_int(out, wrap((int64_t)a->as.integer * b->as.integer));
  } else {
    set_float(out, as_double(a) * as_double(b));
  }
}

void mrt_div(mrt_value* out, const mrt_value* a, const mrt_value* b,
             const mrt_site* site) {
  if (!is_number(a) || !is_number(b)) {
    unsupported(site, "/", a, b);
  }
  if (a->tag == MRT_INT && b->tag == MRT_INT) {
    if (b->as.integer == 0) {
      fail(site, "Division by zero");
    }
    set_int(out, wrap((int64_t)a->as.integer / b->as.integer));
  } else {
    double divisor = as_double(b);
    if (divisor == 0.0) {
      fail(site, "Division by zero");
    }
    set_float(out, as_double(a) / divisor);
  }
}

void mrt_mod(mrt_value* out, const mrt_value* a, const mrt_value* b,
             const mrt_site* site) {
  if (!is_number(a) || !is_number(b)) {
    unsupported(site, "%", a, b);
  }
  if (a->tag == MRT_INT && b->tag == MRT_INT) {
    if (b->as.integer == 0) {
      fail(site, "Division by zero");
    }
    set_int(out, b->as.integer == -1 ? 0 : a->as.integer % b->as.integer);
  } else {
    double divisor = as_double(b);
    if (divisor == 0.0) {
      fail(site, "Division by zero");
    }
    set_float(out, fmod(as_double(a), divisor));
  }
}

void mrt_lt(mrt_value* out, const mrt_value* a, const mrt_value* b,
            const mrt_site* site) {
  if (!is_number(a) || !is_number(b)) {
    unsupported(site, "<", a, b);
  }
  set_int(out, as_double(a) < as_double(b));
}

void mrt_le(mrt_value* out, const mrt_value* a, const mrt_value* b,
            const mrt_site* site) {
  if (!is_number(a) || !is_number(b)) {
    unsupported(site, "<=", a, b);
  }
  set_int(out, as_double(a) <= as_double(b));
}

void mrt_gt(mrt_value* out, const mrt_value* a, const mrt_value* b,
            const mrt_site* site) {
  if (!is_number(a) || !is_number(b)) {
    unsupported(site, ">", a, b);
  }
  set_int(out, as_double(a) > as_double(b));
}

void mrt_ge(mrt_value* out, const mrt_value* a, const mrt_value* b,
            const mrt_site* site) {
  if (!is_number(a) || !is_number(b)) {
    unsupported(site, ">=", a, b);
  }
  set_int(out, as_double(a) >= as_double(b));
}

void mrt_neg(mrt_value* out, const mrt_value* a, const mrt_site* site) {
  if (a->tag == MRT_FLOAT) {
    set_float(out, -a->as.real);
  } else if (a->tag == MRT_INT) {
    set_int(out, (int32_t)(0u - (uint32_t)a->as.integer));
  } else {
    fail(site, "Operator '-' cannot be applied to %s", TYPE_NAMES[a->tag]);
  }
}

void mrt_not(mrt_value* out, const mrt_value* a, const mrt_site* site) {
  if (a->tag != MRT_INT) {
    fail(site, "Operator '!' cannot be applied to %s", TYPE_NAMES[a->tag]);
  }
  set_int(out, a->as.integer == 0);
}

int32_t mrt_condition(const mrt_value* value, const mrt_site* site) {
  if (value->tag != MRT_INT) {
    fail(site, "Condition must be an integer, got %s", TYPE_NAMES[value->tag]);
  }
  return value->as.integer != 0;
}

int32_t mrt_logical(const mrt_value* value, int32_t is_and,
                    const mrt_site* site) {
  if (value->tag != MRT_INT) {
    fail(site, "Operator '%s' cannot be applied to %s", is_and ? "&&" : "||",
         TYPE_NAMES[value->tag]);
  }
  return value->as.integer != 0;
}

void mrt_range(const mrt_value* start, const mrt_value* end, int32_t* from,
               int32_t* to, const mrt_site* site) {
  if (start->tag != MRT_INT || end->tag != MRT_INT) {
    const mrt_value* bad = start->tag == MRT_INT ? end : start;
    fail(site, "Range bounds must be integers, got %s", TYPE_NAMES[bad->tag]);
  }
  *from = start->as.integer;
  *to = end->as.integer;
}

static mrt_array* checked_array(const mrt_value* object, const mrt_value* index,
                                const mrt_site* site, int32_t* position) {
  if (object->tag != MRT_ARRAY) {
    fail(site, "Cannot index %s", TYPE_DESCRIPTIONS[object->tag]);
  }
  if (index->tag != MRT_INT) {
    fail(site, "Array index must be an integer, got %s",
         TYPE_NAMES[index->tag]);
  }
  mrt_array* array = object->as.pointer;
  *position = index->as.integer;
  if (*position < 0 || (uint32_t)*position >= array->length) {
    fail(site, "Array index %d out of bounds for length %u", *position,
         array->length);
  }
  return array;
}

void mrt_index(mrt_value* out, const mrt_value* object, const mrt_value* index,
               const mrt_site* site) {
  int32_t position;
  mrt_array* array = checked_array(object, index, site, &position);
  mrt_copy(out, &array->items[position]);
}

void mrt_index_set(const mrt_value* object, const mrt_value* index,
                   const mrt_value* value, const mrt_site* site) {
  int32_t position;
  mrt_array* array = checked_array(object, index, site, &position);
  mrt_value old = array->items[position];
  mrt_copy(&array->items[position], value);
  mrt_release(&old);
}

static mrt_value* find_field(const mrt_value* object, const char* name,
                             const mrt_site* site) {
  if (object->tag != MRT_OBJECT) {
    fail(site, "Cannot access field '%s' on %s", name,
         TYPE_DESCRIPTIONS[object->tag]);
  }
  mrt_object* record = object->as.pointer;
  for (uint32_t i = 0; i < record->shape->count; ++i) {
    if (strcmp(record->shape->names[i], name) == 0) {
      return &record->fields[i];
    }
  }
  fail(site, "Object has no field '%s'", name);
}

void mrt_field(mrt_value* out, const mrt_value* object, const char* name,
               const mrt_site* site) {
  mrt_copy(out, find_field(object, name, site));
}

void mrt_field_set(const mrt_value* object, const char* name,
                   const mrt_value* value, const mrt_site* site) {
  mrt_value* field = find_field(object, name, site);
  mrt_value old = *field;
  mrt_copy(field, value);
  mrt_release(&old);
}

void mrt_enter(const mrt_site* site) {
  char marker;
  if (call_depth >= MEADOWS_MAX_CALL_DEPTH ||
      (uintptr_t)&marker < stack_limit) {
    fail(site, "Maximum call depth exceeded");
  }
  ++call_depth;
}

void mrt_leave(void) {
  --call_depth;
}

void mrt_call(mrt_value* out, const mrt_value* callee, mrt_value* args,
              uint32_t argc, const mrt_site* site) {
  if (callee->tag != MRT_FUNCTION) {
    fail(site, "Cannot call %s", TYPE_DESCRIPTIONS[callee->tag]);
  }
  const mrt_function* function = callee->as.pointer;
  if (function->arity != argc) {
    char expected[32];
    fail(site, "Function '%s' takes %s, got %u", function->name,
         plural(function->arity, expected, sizeof(expected)), argc);
  }
  mrt_enter(site);
  function->code(out, args);
  mrt_leave();
}

void mrt_print(const mrt_value* value, const mrt_site* site) {
  if (value->tag != MRT_STRING && !is_number(value)) {
    fail(site, "print() cannot print %s", TYPE_DESCRIPTIONS[value->tag]);
  }
  char buffer[32];
  const char* data;
  uint32_t length = text_of(value, buffer, &data);
  fwrite(data, 1, length, stdout);
  fputc('\n', stdout);
}

void mrt_len(mrt_value* out, const mrt_value* value, const mrt_site* site) {
  if (value->tag == MRT_STRING) {
    set_int(out, (int32_t)((const mrt_string*)value->as.pointer)->length);
  } else if (value->tag == MRT_ARRAY) {
    set_int(out, (int32_t)((const mrt_array*)value->as.pointer)->length);
  } else {
    fail(site, "len() requires a string or an array, got %s",
         TYPE_NAMES[value->tag]);
  }
}

void mrt_str(mrt_value* out, const mrt_value* value, const mrt_site* site) {
  if (!is_number(value)) {
    fail(site, "str() requires a number, got %s", TYPE_NAMES[value->tag]);
  }
  char buffer[32];
  const char* data;
  uint32_t length = text_of(value, buffer, &data);
  mrt_string* string = new_string(length);
  memcpy(string->data, data, length);
  set_string(out, string);
}

void mrt_push(mrt_value* out, const mrt_value* array_value,
              const mrt_value* value, const mrt_site* site) {
  if (array_value->tag != MRT_ARRAY) {
    fail(site, "push() requires an array as its first argument, got %s",
         TYPE_NAMES[array_value->tag]);
  }
  mrt_array* array = array_value->as.pointer;
  if (array->length >= MEADOWS_MAX_ARRAY_LENGTH) {
    fail(site, "Array exceeds the maximum length");
  }
  if (array->length == array->capacity) {
    array->capacity = array->capacity ? array->capacity * 2 : 4;
    array->items = realloc(array->items, array->capacity * sizeof(mrt_value));
    if (!array->items) {
      fail(NULL, "Out of memory");
    }
  }
  mrt_copy(&array->items[array->length++], value);
  mrt_copy(out, array_value);
}
