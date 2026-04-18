#ifndef VECTOR_H

#define VECTOR_H

#include <stddef.h>
#include <stdlib.h>

#define VECTOR_SUCCESS 0
#define VECTOR_ERR_ALLOC 1
#define VECTOR_ERR_OUT_OF_RANGE 2
#define VECTOR_ERR_COPY_FAILED 3
#define VECTOR_INVALID_PTR 4
#define VECTOR_SIZE_T_OVERFLOW 5
#define VECTOR_REQ_CAP_INVALID 6
#define VECTOR_SWAP_PARAM_INVALID 7

#define VECTOR_NOT_FOUND_POS ((size_t)-1)

struct Vector {
    size_t elem_size;
    size_t capacity;
    size_t size;
    char* arr;
};
typedef struct Vector Vector ;

Vector* vector_create(const size_t element_size, const size_t initial_capacity);

void vector_free(Vector* vector);

size_t vector_size(const Vector* vector);

size_t vector_capacity(const Vector* vector);

void* vector_arr(const Vector* vector);

int vector_push_back(Vector* vector, const void* value);

int vector_pop_back(Vector* vector, void* out_val);

int vector_insert(Vector* vector, const void* value, size_t index);

int vector_remove(Vector* vector, const size_t index);

char* vector_get(const Vector* vector, const size_t index);

int vector_set(Vector* vector, const void* value, const size_t index);

void vector_clear(Vector* vector);

int vector_reserve(Vector* vector, const size_t req_cap);

int vector_shrink(Vector* vector);

int vector_push_front(Vector* vector, const void* value);

int vector_pop_front(Vector* vector, void* out_val);

Vector* vector_copy(const Vector* vector);

int vector_swap(Vector* vector, const size_t index1, const size_t index2);

size_t vector_find(const Vector* vector, const void* value,
                    int (*cmp)(const void*, const void*));

int ch_vcmp(const void* a, const void* value);

char* vector_get_string(const Vector* vector, size_t index);

#endif // !VECTOR_H

