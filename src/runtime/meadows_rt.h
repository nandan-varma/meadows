#ifndef MEADOWS_RT_H
#define MEADOWS_RT_H

#include <stdint.h>

enum { MRT_INT, MRT_FLOAT, MRT_STRING, MRT_ARRAY, MRT_OBJECT, MRT_FUNCTION };

typedef struct mrt_string {
  uint32_t refs;
  uint32_t length;
  char data[];
} mrt_string;

typedef struct mrt_value {
  uint32_t tag;
  uint32_t reserved;
  union {
    int32_t integer;
    double real;
    void* pointer;
  } as;
} mrt_value;

typedef struct mrt_site {
  const char* file;
  uint32_t line;
  uint32_t column;
} mrt_site;

typedef struct mrt_shape {
  uint32_t count;
  uint32_t reserved;
  const char* names[];
} mrt_shape;

typedef struct mrt_function {
  void (*code)(mrt_value* result, mrt_value* args);
  uint32_t arity;
  const char* name;
} mrt_function;

int mrt_run(void (*entry)(void));

void mrt_make_int(mrt_value* out, int32_t value);
void mrt_make_float(mrt_value* out, double value);
void mrt_make_string(mrt_value* out, const mrt_string* literal);
void mrt_make_function(mrt_value* out, const mrt_function* function);
void mrt_make_array(mrt_value* out, mrt_value* items, uint32_t count);
void mrt_make_object(mrt_value* out, const mrt_shape* shape, mrt_value* fields);

void mrt_copy(mrt_value* out, const mrt_value* value);
void mrt_store(mrt_value* target, mrt_value* value);
void mrt_release(mrt_value* value);

void mrt_add(mrt_value* out, const mrt_value* a, const mrt_value* b,
             const mrt_site* site);
void mrt_sub(mrt_value* out, const mrt_value* a, const mrt_value* b,
             const mrt_site* site);
void mrt_mul(mrt_value* out, const mrt_value* a, const mrt_value* b,
             const mrt_site* site);
void mrt_div(mrt_value* out, const mrt_value* a, const mrt_value* b,
             const mrt_site* site);
void mrt_mod(mrt_value* out, const mrt_value* a, const mrt_value* b,
             const mrt_site* site);
void mrt_eq(mrt_value* out, const mrt_value* a, const mrt_value* b);
void mrt_ne(mrt_value* out, const mrt_value* a, const mrt_value* b);
void mrt_lt(mrt_value* out, const mrt_value* a, const mrt_value* b,
            const mrt_site* site);
void mrt_le(mrt_value* out, const mrt_value* a, const mrt_value* b,
            const mrt_site* site);
void mrt_gt(mrt_value* out, const mrt_value* a, const mrt_value* b,
            const mrt_site* site);
void mrt_ge(mrt_value* out, const mrt_value* a, const mrt_value* b,
            const mrt_site* site);
void mrt_neg(mrt_value* out, const mrt_value* a, const mrt_site* site);
void mrt_not(mrt_value* out, const mrt_value* a, const mrt_site* site);

int32_t mrt_condition(const mrt_value* value, const mrt_site* site);
int32_t mrt_logical(const mrt_value* value, int32_t is_and,
                    const mrt_site* site);
void mrt_range(const mrt_value* start, const mrt_value* end, int32_t* from,
               int32_t* to, const mrt_site* site);

void mrt_index(mrt_value* out, const mrt_value* object, const mrt_value* index,
               const mrt_site* site);
void mrt_index_set(const mrt_value* object, const mrt_value* index,
                   const mrt_value* value, const mrt_site* site);
void mrt_field(mrt_value* out, const mrt_value* object, const char* name,
               const mrt_site* site);
void mrt_field_set(const mrt_value* object, const char* name,
                   const mrt_value* value, const mrt_site* site);

void mrt_enter(const mrt_site* site);
void mrt_leave(void);
void mrt_call(mrt_value* out, const mrt_value* callee, mrt_value* args,
              uint32_t argc, const mrt_site* site);

void mrt_print(const mrt_value* value, const mrt_site* site);
void mrt_len(mrt_value* out, const mrt_value* value, const mrt_site* site);
void mrt_str(mrt_value* out, const mrt_value* value, const mrt_site* site);
void mrt_push(mrt_value* out, const mrt_value* array, const mrt_value* value,
              const mrt_site* site);

#endif
